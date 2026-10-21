#include "VoxelTerrainComponent.h"
#include "VoxelSceneProxy.h"
#include "VoxelGenerator.h"
#include "VoxelChunkData.h"
#include "TransvoxelMesher.h"
#include "RenderingThread.h"
#include "ProceduralMeshComponent.h"
#include "Materials/Material.h"
#include "MaterialDomain.h"
#include "Async/Async.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "HAL/PlatformTime.h"

UVoxelTerrainComponent::UVoxelTerrainComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    bUseAsOccluder = false;
    CastShadow = true;
    LODRatios = { 0.5f };
}

void UVoxelTerrainComponent::BeginPlay()
{
    Super::BeginPlay();

    if (TerrainMaterial) TerrainMaterial->EnsureIsComplete();

    LODRatios.Sort();
    LODRatios.RemoveAll([](float R) { return R <= 0.f || R >= 1.f; });

    ChunkMeshes.Reserve(4096);
    StagingMeshes.Reserve(1024);
    CachedDesiredChunks.Reserve(4096);
    ChunkCollisionProxies.Reserve(64);
    PendingCollisionAdds.Reserve(64);
    PendingCollisionRemoves.Reserve(64);

    TickHandle = FTSTicker::GetCoreTicker().AddTicker(
        FTickerDelegate::CreateWeakLambda(this, [this](float)
            {
                StreamTick();
                return true;
            }), 0.1f);

    CollisionTickHandle = FTSTicker::GetCoreTicker().AddTicker(
        FTickerDelegate::CreateWeakLambda(this, [this](float)
            {
                ProcessCollisionQueue();
                return true;
            }), 0.1f);
}

FPrimitiveSceneProxy* UVoxelTerrainComponent::CreateSceneProxy()
{
    return new FVoxelSceneProxy(this, TerrainMaterial);
}

FBoxSphereBounds UVoxelTerrainComponent::CalcBounds(const FTransform& LocalToWorld) const
{
    FBox Box = LocalBounds.IsValid ? LocalBounds : FBox(FVector(-100), FVector(100));
    return FBoxSphereBounds(Box).TransformBy(LocalToWorld);
}

void UVoxelTerrainComponent::GetUsedMaterials(
    TArray<UMaterialInterface*>& OutMaterials,
    bool bGetDebugMaterials) const
{
    if (TerrainMaterial) OutMaterials.Add(TerrainMaterial);
    else if (UMaterial* DefaultMat = UMaterial::GetDefaultMaterial(MD_Surface))
        OutMaterials.Add(DefaultMat);
}

FVector UVoxelTerrainComponent::ChunkOriginWorld(const FChunkKey& Key) const
{
    const float S = (float)(1 << Key.LOD);
    const float ChunkWorldSize =
        VoxelConfig::ChunkSize * VoxelConfig::VoxelSize * S;
    return FVector(Key.X, Key.Y, Key.Z) * ChunkWorldSize;
}

int32 UVoxelTerrainComponent::SelectLOD(
    const FVector& ChunkCenter, const FVector& CamLoc) const
{
    if (LODRatios.Num() == 0) return 0;

    const FVector Delta = ChunkCenter - CamLoc;
    const float DistXY = FMath::Sqrt(Delta.X * Delta.X + Delta.Y * Delta.Y);
    const float NormXY = DistXY / FMath::Max(1.f, LOD0RadiusXYWorld);
    const float NormZ = FMath::Abs(Delta.Z) / FMath::Max(1.f, LOD0RadiusZWorld);
    const float Norm = FMath::Max(NormXY, NormZ);

    int32 LOD = 0;
    for (float Ratio : LODRatios)
    {
        if (Norm >= Ratio) ++LOD;
        else break;
    }
    return LOD;
}

void UVoxelTerrainComponent::ComputeDesiredChunks(
    const FVector& CamLoc, TSet<FChunkKey>& OutDesired) const
{
    OutDesired.Reset();

    const float BaseChunkWorld =
        VoxelConfig::ChunkSize * VoxelConfig::VoxelSize;
    const FIntVector Base = WorldToBaseChunk(CamLoc);

    struct FPosInfo { int32 TargetLOD = 0; bool bCovered = false; };
    TMap<FIntVector, FPosInfo> Positions;
    Positions.Reserve(4096);

    const int32 MaxRXY = FMath::CeilToInt(LOD0RadiusXYWorld / BaseChunkWorld);
    const int32 MaxRZ = FMath::CeilToInt(LOD0RadiusZWorld / BaseChunkWorld);
    const float RadiusXY2 = LOD0RadiusXYWorld * LOD0RadiusXYWorld;

    for (int32 dx = -MaxRXY; dx <= MaxRXY; ++dx)
        for (int32 dy = -MaxRXY; dy <= MaxRXY; ++dy)
            for (int32 dz = -MaxRZ; dz <= MaxRZ; ++dz)
            {
                const int32 X = Base.X + dx;
                const int32 Y = Base.Y + dy;
                const int32 Z = Base.Z + dz;

                const FVector ChunkCenter(
                    (X + 0.5f) * BaseChunkWorld,
                    (Y + 0.5f) * BaseChunkWorld,
                    (Z + 0.5f) * BaseChunkWorld);
                const FVector Delta = ChunkCenter - CamLoc;

                const float DistXY2 = Delta.X * Delta.X + Delta.Y * Delta.Y;
                if (DistXY2 > RadiusXY2) continue;
                if (FMath::Abs(Delta.Z) > LOD0RadiusZWorld) continue;

                FPosInfo Info;
                Info.TargetLOD = SelectLOD(ChunkCenter, CamLoc);
                Positions.Add(FIntVector(X, Y, Z), Info);
            }

    if (Positions.Num() == 0) return;

    const int32 MaxLOD = LODRatios.Num();

    for (int32 L = MaxLOD; L >= 1; --L)
    {
        const int32 S = 1 << L;
        TSet<FIntVector> CandidateParents;
        for (auto& Pair : Positions)
        {
            if (Pair.Value.bCovered) continue;
            if (Pair.Value.TargetLOD < L) continue;
            const FIntVector& P = Pair.Key;
            CandidateParents.Add(FIntVector(
                FMath::FloorToInt((float)P.X / S),
                FMath::FloorToInt((float)P.Y / S),
                FMath::FloorToInt((float)P.Z / S)));
        }

        for (const FIntVector& Parent : CandidateParents)
        {
            bool bAllValid = true;
            TArray<FIntVector> CoveredChildren;
            CoveredChildren.Reserve(S * S * S);

            for (int32 i = 0; i < S && bAllValid; ++i)
                for (int32 j = 0; j < S && bAllValid; ++j)
                    for (int32 k = 0; k < S && bAllValid; ++k)
                    {
                        const FIntVector Child(
                            Parent.X * S + i, Parent.Y * S + j, Parent.Z * S + k);

                        const FPosInfo* Info = Positions.Find(Child);
                        if (Info)
                        {
                            if (Info->bCovered || Info->TargetLOD < L)
                            {
                                bAllValid = false;
                                break;
                            }
                            CoveredChildren.Add(Child);
                        }
                    }

            if (!bAllValid) continue;

            OutDesired.Add(FChunkKey(Parent.X, Parent.Y, Parent.Z, L));
            for (const FIntVector& Child : CoveredChildren)
                Positions[Child].bCovered = true;
        }
    }

    for (auto& Pair : Positions)
    {
        if (Pair.Value.bCovered) continue;
        const FIntVector& P = Pair.Key;
        OutDesired.Add(FChunkKey(P.X, P.Y, P.Z, 0));
    }
}

void UVoxelTerrainComponent::GenerateOneChunk(const FChunkKey& Key)
{
    if (ChunkMeshes.Contains(Key)) return;

    FVoxelGenerator Gen;
    FVoxelChunkData ChunkData;
    Gen.GenerateChunk(Key.X, Key.Y, Key.Z, Key.LOD, ChunkData.Voxels.GetData());

    auto MeshData = MakeShared<FVoxelMeshData, ESPMode::ThreadSafe>();
    FTransvoxelMesher::GenerateRegular(
        ChunkData, VoxelConfig::VoxelSize, Key.LOD, *MeshData);

    const FVector Offset = ChunkOriginWorld(Key);
    const FVector3f OffsetF((float)Offset.X, (float)Offset.Y, (float)Offset.Z);
    for (FVector3f& P : MeshData->Positions) P += OffsetF;

    ChunkMeshes.Add(Key, MeshData);
}

bool UVoxelTerrainComponent::CollectReplacementFor(
    const FChunkKey& OldKey,
    TArray<TPair<FChunkKey, TSharedPtr<FVoxelMeshData, ESPMode::ThreadSafe>>>& OutNewChunks) const
{
    OutNewChunks.Reset();

    if (OldKey.LOD > 0)
    {
        for (int32 i = 0; i < 2; ++i)
            for (int32 j = 0; j < 2; ++j)
                for (int32 k = 0; k < 2; ++k)
                {
                    const FChunkKey CK(
                        OldKey.X * 2 + i, OldKey.Y * 2 + j, OldKey.Z * 2 + k,
                        OldKey.LOD - 1);
                    auto* Staged = StagingMeshes.Find(CK);
                    if (!Staged || !Staged->IsValid()) return false;
                    OutNewChunks.Add(TPair<FChunkKey, TSharedPtr<FVoxelMeshData, ESPMode::ThreadSafe>>(CK, *Staged));
                }
        return OutNewChunks.Num() == 8;
    }
    else
    {
        const FChunkKey PK(
            FMath::FloorToInt((float)OldKey.X / 2),
            FMath::FloorToInt((float)OldKey.Y / 2),
            FMath::FloorToInt((float)OldKey.Z / 2),
            OldKey.LOD + 1);
        auto* Staged = StagingMeshes.Find(PK);
        if (!Staged || !Staged->IsValid()) return false;
        OutNewChunks.Add(TPair<FChunkKey, TSharedPtr<FVoxelMeshData, ESPMode::ThreadSafe>>(PK, *Staged));
        return true;
    }
}

void UVoxelTerrainComponent::ExecuteAtomicSwitch(
    const TArray<FChunkKey>& OldKeys,
    const TArray<TPair<FChunkKey, TSharedPtr<FVoxelMeshData, ESPMode::ThreadSafe>>>& NewChunks)
{
    for (const FChunkKey& K : OldKeys)
    {
        ChunkMeshes.Remove(K);
        ChunkLastActiveTime.Remove(K);
        RemoveCollisionForChunk(K);
    }
    for (auto& Item : NewChunks)
    {
        ChunkMeshes.Add(Item.Key, Item.Value);
        ChunkLastActiveTime.Add(Item.Key, FPlatformTime::Seconds());
        StagingMeshes.Remove(Item.Key);
    }

    bBoundsDirty = true;

    FVoxelSceneProxy* Proxy = static_cast<FVoxelSceneProxy*>(SceneProxy);
    if (Proxy)
    {
        TArray<FChunkKey> Removes = OldKeys;
        TArray<TPair<FChunkKey, TSharedPtr<FVoxelMeshData, ESPMode::ThreadSafe>>> Adds = NewChunks;
        ENQUEUE_RENDER_COMMAND(LODAtomicSwitch)(
            [Proxy, Adds, Removes](FRHICommandListImmediate& RHICmdList)
            {
                for (auto& Item : Adds)
                    Proxy->AddChunk_RenderThread(RHICmdList, Item.Key, Item.Value);
                for (const FChunkKey& K : Removes)
                    Proxy->RemoveChunk_RenderThread(RHICmdList, K);
            });
    }
}

void UVoxelTerrainComponent::ProcessLODSwitches()
{
    TArray<FChunkKey> OldKeys;
    OldKeys.Reserve(ChunkMeshes.Num());
    for (auto& Pair : ChunkMeshes) OldKeys.Add(Pair.Key);

    TSet<FChunkKey> HandledNewKeys;

    for (const FChunkKey& OldKey : OldKeys)
    {
        if (!ChunkMeshes.Contains(OldKey)) continue;

        TArray<TPair<FChunkKey, TSharedPtr<FVoxelMeshData, ESPMode::ThreadSafe>>> NewChunks;
        if (!CollectReplacementFor(OldKey, NewChunks)) continue;

        bool bAllNew = true;
        for (auto& N : NewChunks)
        {
            if (HandledNewKeys.Contains(N.Key)) { bAllNew = false; break; }
        }
        if (!bAllNew) continue;

        if (OldKey.LOD == 0)
        {
            const FChunkKey PK = NewChunks[0].Key;
            bool bAllOldReady = true;
            TArray<FChunkKey> OldSiblings;
            OldSiblings.Reserve(8);
            for (int32 i = 0; i < 2; ++i)
                for (int32 j = 0; j < 2; ++j)
                    for (int32 k = 0; k < 2; ++k)
                    {
                        const FChunkKey CK(
                            PK.X * 2 + i, PK.Y * 2 + j, PK.Z * 2 + k, 0);
                        if (!ChunkMeshes.Contains(CK)) { bAllOldReady = false; break; }
                        OldSiblings.Add(CK);
                    }
            if (!bAllOldReady) continue;

            ExecuteAtomicSwitch(OldSiblings, NewChunks);
            HandledNewKeys.Add(PK);
        }
        else
        {
            ExecuteAtomicSwitch({ OldKey }, NewChunks);
            for (auto& N : NewChunks) HandledNewKeys.Add(N.Key);
        }
    }
}

void UVoxelTerrainComponent::StreamTick()
{
    UWorld* World = GetWorld();
    if (!World) return;

    FVector CamLoc = GetComponentLocation();
    if (APlayerController* PC = World->GetFirstPlayerController())
    {
        if (APawn* Pawn = PC->GetPawnOrSpectator())
            CamLoc = Pawn->GetActorLocation();
    }

    const double Now = FPlatformTime::Seconds();
    TWeakObjectPtr<UVoxelTerrainComponent> WeakThis(this);

    // ---------- 1) 期望 chunk 集合（缓存，相机移动超过 100 单位才重算） ----------
    if (!CamLoc.Equals(LastDesiredCamLoc, 100.f))
    {
        ComputeDesiredChunks(CamLoc, CachedDesiredChunks);
        LastDesiredCamLoc = CamLoc;
    }
    const TSet<FChunkKey>& DesiredChunks = CachedDesiredChunks;

    // ---------- 2) 清理过时的 staging ----------
    {
        TArray<FChunkKey> StaleStaging;
        for (auto& Pair : StagingMeshes)
            if (!DesiredChunks.Contains(Pair.Key))
                StaleStaging.Add(Pair.Key);

        for (const FChunkKey& K : StaleStaging)
        {
            StagingMeshes.Remove(K);
            RequestedChunks.Remove(K);
        }
    }

    // ---------- 3) 加载：控制在途上限 ----------
    int32 NewRequests = 0;
    const int32 MaxInFlight = FMath::Max(1, MaxInFlightRequests);
    for (const FChunkKey& Key : DesiredChunks)
    {
        // 在途已满，停止发新请求（下一 tick 会继续）
        if (RequestedChunks.Num() >= MaxInFlight) break;

        if (ChunkMeshes.Contains(Key) || StagingMeshes.Contains(Key))
        {
            ChunkLastActiveTime.Add(Key, Now);
            continue;
        }
        if (RequestedChunks.Contains(Key)) continue;

        RequestedChunks.Add(Key);
        ++NewRequests;

        Async(EAsyncExecution::ThreadPool, [WeakThis, Key]()
            {
                FVoxelGenerator Gen;
                FVoxelChunkData ChunkData;
                Gen.GenerateChunk(Key.X, Key.Y, Key.Z, Key.LOD,
                    ChunkData.Voxels.GetData());

                auto MeshData = MakeShared<FVoxelMeshData, ESPMode::ThreadSafe>();
                FTransvoxelMesher::GenerateRegular(
                    ChunkData, VoxelConfig::VoxelSize, Key.LOD, *MeshData);

                const float S = (float)(1 << Key.LOD);
                const float LODChunkWorldSize =
                    VoxelConfig::ChunkSize * VoxelConfig::VoxelSize * S;
                const FVector3f Offset(
                    Key.X * LODChunkWorldSize,
                    Key.Y * LODChunkWorldSize,
                    Key.Z * LODChunkWorldSize);
                for (FVector3f& P : MeshData->Positions) P += Offset;

                AsyncTask(ENamedThreads::GameThread, [WeakThis, Key, MeshData]()
                    {
                        if (auto* Self = WeakThis.Get())
                            Self->OnChunkGenerated(Key, MeshData);
                    });
            });
    }

    if (NewRequests > 0)
    {
        UE_LOG(LogTemp, Log,
            TEXT("VoxelTerrain: +%d chunks (showing=%d, staging=%d, inFlight=%d, desired=%d)"),
            NewRequests, ChunkMeshes.Num(), StagingMeshes.Num(),
            RequestedChunks.Num(), DesiredChunks.Num());
    }

    // ---------- 4) LOD 原子切换 ----------
    ProcessLODSwitches();

    // ---------- 5) 卸载 ----------
    TArray<FChunkKey> ToUnload;
    for (auto& Pair : ChunkMeshes)
    {
        const FChunkKey& Key = Pair.Key;
        if (DesiredChunks.Contains(Key)) continue;

        bool bHasPendingReplacement = false;
        if (Key.LOD > 0)
        {
            for (int32 i = 0; i < 2 && !bHasPendingReplacement; ++i)
                for (int32 j = 0; j < 2 && !bHasPendingReplacement; ++j)
                    for (int32 k = 0; k < 2 && !bHasPendingReplacement; ++k)
                    {
                        const FChunkKey CK(Key.X * 2 + i, Key.Y * 2 + j, Key.Z * 2 + k, Key.LOD - 1);
                        if (StagingMeshes.Contains(CK)) bHasPendingReplacement = true;
                    }
        }
        else
        {
            const FChunkKey PK(
                FMath::FloorToInt((float)Key.X / 2),
                FMath::FloorToInt((float)Key.Y / 2),
                FMath::FloorToInt((float)Key.Z / 2),
                Key.LOD + 1);
            if (StagingMeshes.Contains(PK)) bHasPendingReplacement = true;
        }

        if (bHasPendingReplacement) continue;

        if (const double* LastTime = ChunkLastActiveTime.Find(Key))
        {
            if (Now - *LastTime < UnloadDelay) continue;
        }
        ToUnload.Add(Key);
    }

    for (const FChunkKey& Key : ToUnload)
    {
        ChunkMeshes.Remove(Key);
        ChunkLastActiveTime.Remove(Key);
        RequestedChunks.Remove(Key);
        RemoveCollisionForChunk(Key);

        FVoxelSceneProxy* Proxy = static_cast<FVoxelSceneProxy*>(SceneProxy);
        if (Proxy)
        {
            ENQUEUE_RENDER_COMMAND(RemoveVoxelChunk)(
                [Proxy, Key](FRHICommandListImmediate& RHICmdList)
                {
                    Proxy->RemoveChunk_RenderThread(RHICmdList, Key);
                });
        }
    }

    if (ToUnload.Num() > 0) bBoundsDirty = true;

    UpdateCollisions(CamLoc);

    if (bBoundsDirty)
    {
        bBoundsDirty = false;
        UpdateBounds();
        MarkRenderTransformDirty();
    }
}

void UVoxelTerrainComponent::OnChunkGenerated(
    FChunkKey Key,
    TSharedPtr<FVoxelMeshData, ESPMode::ThreadSafe> Mesh)
{
    if (!RequestedChunks.Contains(Key)) return;
    RequestedChunks.Remove(Key);

    if (!Mesh.IsValid()) return;
    if (ChunkMeshes.Contains(Key) || StagingMeshes.Contains(Key)) return;

    bool bNeedsStaging = false;

    if (Key.LOD > 0)
    {
        for (int32 i = 0; i < 2 && !bNeedsStaging; ++i)
            for (int32 j = 0; j < 2 && !bNeedsStaging; ++j)
                for (int32 k = 0; k < 2 && !bNeedsStaging; ++k)
                {
                    const FChunkKey CK(Key.X * 2 + i, Key.Y * 2 + j, Key.Z * 2 + k, Key.LOD - 1);
                    if (ChunkMeshes.Contains(CK)) bNeedsStaging = true;
                }
    }
    else
    {
        const FChunkKey PK(
            FMath::FloorToInt((float)Key.X / 2),
            FMath::FloorToInt((float)Key.Y / 2),
            FMath::FloorToInt((float)Key.Z / 2),
            1);
        if (ChunkMeshes.Contains(PK)) bNeedsStaging = true;
    }

    if (bNeedsStaging)
    {
        StagingMeshes.Add(Key, Mesh);
        return;
    }

    ChunkMeshes.Add(Key, Mesh);
    ChunkLastActiveTime.Add(Key, FPlatformTime::Seconds());

    // ⭐ 优化：用理论 AABB（chunk 世界原点 + 世界尺寸），O(1) 计算，不遍历顶点
    {
        const float CS = VoxelConfig::ChunkSize * VoxelConfig::VoxelSize
            * (float)(1 << Key.LOD);
        const FVector Origin(
            Key.X * CS, Key.Y * CS, Key.Z * CS);
        const FVector Extent(CS, CS, CS);
        LocalBounds += FBox(Origin, Origin + Extent);
    }
    bBoundsDirty = true;

    FVoxelSceneProxy* Proxy = static_cast<FVoxelSceneProxy*>(SceneProxy);
    if (Proxy)
    {
        ENQUEUE_RENDER_COMMAND(AddVoxelChunk)(
            [Proxy, Key, Mesh](FRHICommandListImmediate& RHICmdList)
            {
                Proxy->AddChunk_RenderThread(RHICmdList, Key, Mesh);
            });
    }
}

void UVoxelTerrainComponent::EndPlay(const EEndPlayReason::Type Reason)
{
    if (TickHandle.IsValid()) FTSTicker::GetCoreTicker().RemoveTicker(TickHandle);
    TickHandle.Reset();
    if (CollisionTickHandle.IsValid()) FTSTicker::GetCoreTicker().RemoveTicker(CollisionTickHandle);
    CollisionTickHandle.Reset();
    ClearAllCollisions();
    Super::EndPlay(Reason);
}

FIntVector UVoxelTerrainComponent::WorldToBaseChunk(const FVector& WorldPos) const
{
    const float BaseChunkSize =
        VoxelConfig::ChunkSize * VoxelConfig::VoxelSize;
    return FIntVector(
        FMath::FloorToInt(WorldPos.X / BaseChunkSize),
        FMath::FloorToInt(WorldPos.Y / BaseChunkSize),
        FMath::FloorToInt(WorldPos.Z / BaseChunkSize));
}

void UVoxelTerrainComponent::CreateCollisionForChunk(
    const FChunkKey& Key, const FVoxelMeshData& Mesh)
{
    if (!bGenerateCollision) return;
    if (Mesh.Positions.Num() == 0 || Mesh.Indices.Num() == 0) return;
    if (ChunkCollisionProxies.Contains(Key)) return;

    UProceduralMeshComponent* NewPMC = NewObject<UProceduralMeshComponent>(
        this, NAME_None, RF_Transient);
    if (!NewPMC) return;

    NewPMC->SetupAttachment(this);
    NewPMC->bUseAsyncCooking = true;
    NewPMC->bUseComplexAsSimpleCollision = true;
    NewPMC->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    NewPMC->SetCollisionObjectType(ECC_WorldStatic);
    NewPMC->SetCollisionResponseToAllChannels(ECR_Block);
    NewPMC->SetGenerateOverlapEvents(false);
    NewPMC->SetVisibility(false);
    NewPMC->SetHiddenInGame(true);
    NewPMC->RegisterComponent();

    const int32 NumVerts = Mesh.Positions.Num();
    TArray<FVector>          Verts;
    TArray<int32>            Tris;
    TArray<FVector>          Normals;
    TArray<FVector2D>        UVs;
    TArray<FLinearColor>     LinearColors;
    TArray<FProcMeshTangent> Tangents;

    Verts.SetNumUninitialized(NumVerts);
    Normals.SetNumUninitialized(NumVerts);
    UVs.SetNumUninitialized(NumVerts);
    LinearColors.SetNumUninitialized(NumVerts);
    Tangents.SetNumUninitialized(NumVerts);

    for (int32 i = 0; i < NumVerts; ++i)
    {
        Verts[i] = FVector(Mesh.Positions[i]);
        Normals[i] = Mesh.Normals.IsValidIndex(i) ? FVector(Mesh.Normals[i]) : FVector(0, 0, 1);
        UVs[i] = FVector2D::ZeroVector;
        FColor C = Mesh.Colors.IsValidIndex(i) ? Mesh.Colors[i] : FColor::White;
        LinearColors[i] = FLinearColor(C);
        Tangents[i] = FProcMeshTangent(1, 0, 0);
    }

    Tris.SetNumUninitialized(Mesh.Indices.Num());
    for (int32 i = 0; i < Mesh.Indices.Num(); ++i) Tris[i] = (int32)Mesh.Indices[i];

    NewPMC->CreateMeshSection_LinearColor(
        0, Verts, Tris, Normals, UVs, LinearColors, Tangents, true, true);

    ChunkCollisionProxies.Add(Key, NewPMC);
}

void UVoxelTerrainComponent::RemoveCollisionForChunk(const FChunkKey& Key)
{
    UProceduralMeshComponent* PMC = ChunkCollisionProxies.FindRef(Key);
    if (IsValid(PMC))
    {
        PMC->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        PMC->ClearAllMeshSections();
        PMC->DestroyComponent();
    }
    ChunkCollisionProxies.Remove(Key);
}

void UVoxelTerrainComponent::ClearAllCollisions()
{
    for (auto& Pair : ChunkCollisionProxies)
    {
        if (IsValid(Pair.Value))
        {
            Pair.Value->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            Pair.Value->ClearAllMeshSections();
            Pair.Value->DestroyComponent();
        }
    }
    ChunkCollisionProxies.Empty();
    PendingCollisionAdds.Empty();
    PendingCollisionRemoves.Empty();
}

void UVoxelTerrainComponent::UpdateCollisions(const FVector& CamLoc)
{
    if (!bGenerateCollision) return;

    const float BaseChunkSize =
        VoxelConfig::ChunkSize * VoxelConfig::VoxelSize;
    const float RadiusWorld = (CollisionRadius + 0.5f) * BaseChunkSize;
    const float RadiusSquared = RadiusWorld * RadiusWorld;

    for (auto& Pair : ChunkMeshes)
    {
        const FChunkKey& Key = Pair.Key;
        if (Key.LOD != 0) continue;

        const FVector ChunkCenter(
            (Key.X + 0.5f) * BaseChunkSize,
            (Key.Y + 0.5f) * BaseChunkSize,
            (Key.Z + 0.5f) * BaseChunkSize);

        const bool bShouldHave =
            FVector::DistSquared(ChunkCenter, CamLoc) < RadiusSquared;

        const bool bHas = ChunkCollisionProxies.Contains(Key);
        const bool bPendingAdd = PendingCollisionAdds.Contains(Key);
        const bool bPendingRemove = PendingCollisionRemoves.Contains(Key);

        if (bShouldHave && !bHas && !bPendingAdd)
            PendingCollisionAdds.Add(Key);
        else if (!bShouldHave && bHas && !bPendingRemove)
            PendingCollisionRemoves.Add(Key);
    }
}

void UVoxelTerrainComponent::ProcessCollisionQueue()
{
    const double StartTime = FPlatformTime::Seconds();
    const double MaxBudget = 0.003;

    int32 RemoveBudget = 2;
    while (PendingCollisionRemoves.Num() > 0 && RemoveBudget > 0)
    {
        if (FPlatformTime::Seconds() - StartTime > MaxBudget) break;
        FChunkKey Key = PendingCollisionRemoves.Pop();
        RemoveCollisionForChunk(Key);
        --RemoveBudget;
    }

    int32 AddBudget = 1;
    while (PendingCollisionAdds.Num() > 0 && AddBudget > 0)
    {
        if (FPlatformTime::Seconds() - StartTime > MaxBudget) break;
        FChunkKey Key = PendingCollisionAdds.Pop();
        --AddBudget;

        TSharedPtr<FVoxelMeshData, ESPMode::ThreadSafe>* MeshPtr = ChunkMeshes.Find(Key);
        if (!MeshPtr || !MeshPtr->IsValid()) continue;
        CreateCollisionForChunk(Key, **MeshPtr);
    }
}