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
}

void UVoxelTerrainComponent::BeginPlay()
{
    Super::BeginPlay();

    if (TerrainMaterial)
    {
        TerrainMaterial->EnsureIsComplete();
    }

    ChunkMeshes.Reserve(1024);
    ChunkCollisionProxies.Reserve(64);
    PendingCollisionAdds.Reserve(64);
    PendingCollisionRemoves.Reserve(64);

    TickHandle = FTSTicker::GetCoreTicker().AddTicker(
        FTickerDelegate::CreateWeakLambda(this, [this](float)
            {
                StreamTick();
                return true;
            }), 0.2f);

    // 每帧处理碰撞队列
    CollisionTickHandle = FTSTicker::GetCoreTicker().AddTicker(
        FTickerDelegate::CreateWeakLambda(this, [this](float)
            {
                ProcessCollisionQueue();
                return true;
            }), 0.f);
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
    if (TerrainMaterial)
    {
        OutMaterials.Add(TerrainMaterial);
    }
    else
    {
        if (UMaterial* DefaultMat = UMaterial::GetDefaultMaterial(MD_Surface))
            OutMaterials.Add(DefaultMat);
    }
}

// ============================================================
// 世界原点（含 LOD 缩放）
// ============================================================
FVector UVoxelTerrainComponent::ChunkOriginWorld(const FChunkKey& Key) const
{
    const float S = (float)(1 << Key.LOD);
    const float ChunkWorldSize =
        VoxelConfig::ChunkSize * VoxelConfig::VoxelSize * S;
    return FVector(Key.X, Key.Y, Key.Z) * ChunkWorldSize;
}

// ============================================================
// 单个 chunk 生成（同步版本，未使用，保留兼容）
// ============================================================
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
    const FVector3f OffsetF(
        (float)Offset.X, (float)Offset.Y, (float)Offset.Z);
    for (FVector3f& P : MeshData->Positions)
        P += OffsetF;

    ChunkMeshes.Add(Key, MeshData);
}

// ============================================================
// 主循环：加载 + 卸载
// ============================================================


void UVoxelTerrainComponent::StreamTick()
{
    UWorld* World = GetWorld();
    if (!World) return;

    FVector CamLoc = GetComponentLocation();
    if (APlayerController* PC = World->GetFirstPlayerController())
    {
        if (APawn* Pawn = PC->GetPawnOrSpectator())
        {
            CamLoc = Pawn->GetActorLocation();
        }
    }

    const double Now = FPlatformTime::Seconds();
    TWeakObjectPtr<UVoxelTerrainComponent> WeakThis(this);

    auto RequestChunk = [this, WeakThis, Now](const FChunkKey& Key)
        {
            if (ChunkMeshes.Contains(Key))
            {
                ChunkLastActiveTime.Add(Key, Now);
                return;
            }
            if (RequestedChunks.Contains(Key)) return;
            RequestedChunks.Add(Key);

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
                    for (FVector3f& P : MeshData->Positions)
                        P += Offset;

                    AsyncTask(ENamedThreads::GameThread, [WeakThis, Key, MeshData]()
                        {
                            if (auto* Self = WeakThis.Get())
                                Self->OnChunkGenerated(Key, MeshData);
                        });
                });
        };

    const FIntVector Base = WorldToBaseChunk(CamLoc);

    const float ChunkSizeWorld =
        VoxelConfig::ChunkSize * VoxelConfig::VoxelSize;   // 3200

    // 遍历范围：按世界半径换算成最大 chunk 数
    const int32 MaxRXY =
        FMath::CeilToInt(LOD0RadiusXYWorld / ChunkSizeWorld);
    const int32 MaxRZ =
        FMath::CeilToInt(LOD0RadiusZWorld / ChunkSizeWorld);

    // ---------- 1) 加载：XY 和 Z 各自独立判定 ----------
    for (int32 dx = -MaxRXY; dx <= MaxRXY; ++dx)
    {
        for (int32 dy = -MaxRXY; dy <= MaxRXY; ++dy)
        {
            for (int32 dz = -MaxRZ; dz <= MaxRZ; ++dz)
            {
                const int32 X = Base.X + dx;
                const int32 Y = Base.Y + dy;
                const int32 Z = Base.Z + dz;

                // chunk 世界中心
                const FVector ChunkCenter(
                    (X + 0.5f) * ChunkSizeWorld,
                    (Y + 0.5f) * ChunkSizeWorld,
                    (Z + 0.5f) * ChunkSizeWorld);

                const FVector Delta = ChunkCenter - CamLoc;

                // ⭐ 圆柱形：XY 用圆，Z 用固定范围
                const float DistXY2 = Delta.X * Delta.X + Delta.Y * Delta.Y;
                const float RadiusXY2 = LOD0RadiusXYWorld * LOD0RadiusXYWorld;

                if (DistXY2 > RadiusXY2)
                    continue;

                if (FMath::Abs(Delta.Z) > LOD0RadiusZWorld)
                    continue;

                FChunkKey Key(X, Y, Z, 0);
                RequestChunk(Key);
            }
        }
    }

    // ---------- 2) 卸载：同一套判定 ----------
    TArray<FChunkKey> ToUnload;

    for (auto& Pair : ChunkMeshes)
    {
        const FChunkKey& Key = Pair.Key;

        if (Key.LOD != 0) continue;

        const FVector ChunkCenter(
            (Key.X + 0.5f) * ChunkSizeWorld,
            (Key.Y + 0.5f) * ChunkSizeWorld,
            (Key.Z + 0.5f) * ChunkSizeWorld);

        const FVector Delta = ChunkCenter - CamLoc;

        const float DistXY2 = Delta.X * Delta.X + Delta.Y * Delta.Y;
        const float RadiusXY2 = LOD0RadiusXYWorld * LOD0RadiusXYWorld;

        const bool bInsideXY = DistXY2 <= RadiusXY2;
        const bool bInsideZ = FMath::Abs(Delta.Z) <= LOD0RadiusZWorld;

        if (bInsideXY && bInsideZ)
            continue;

        if (const double* LastTime = ChunkLastActiveTime.Find(Key))
        {
            if (Now - *LastTime < UnloadDelay) continue;
        }

        ToUnload.Add(Key);
    }

    // ---------- 3) 执行卸载 ----------
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

    if (ToUnload.Num() > 0)
    {
        bBoundsDirty = true;
    }

    UpdateCollisions(CamLoc);

    if (bBoundsDirty)
    {
        bBoundsDirty = false;
        UpdateBounds();
        MarkRenderTransformDirty();
    }
}






// ============================================================
// 回调
// ============================================================
void UVoxelTerrainComponent::OnChunkGenerated(
    FChunkKey Key,
    TSharedPtr<FVoxelMeshData, ESPMode::ThreadSafe> Mesh)
{
    if (!Mesh.IsValid()) return;
    if (ChunkMeshes.Contains(Key)) return;

    ChunkMeshes.Add(Key, Mesh);
    ChunkLastActiveTime.Add(Key, FPlatformTime::Seconds());

    for (const FVector3f& P : Mesh->Positions)
        LocalBounds += FVector(P);
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
    if (TickHandle.IsValid())
        FTSTicker::GetCoreTicker().RemoveTicker(TickHandle);
    TickHandle.Reset();

    if (CollisionTickHandle.IsValid())
        FTSTicker::GetCoreTicker().RemoveTicker(CollisionTickHandle);
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

// ============================================================
// 碰撞
// ============================================================
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
        Normals[i] = Mesh.Normals.IsValidIndex(i)
            ? FVector(Mesh.Normals[i]) : FVector(0, 0, 1);
        UVs[i] = FVector2D::ZeroVector;

        FColor C = Mesh.Colors.IsValidIndex(i) ? Mesh.Colors[i] : FColor::White;
        LinearColors[i] = FLinearColor(C);

        Tangents[i] = FProcMeshTangent(1, 0, 0);
    }

    Tris.SetNumUninitialized(Mesh.Indices.Num());
    for (int32 i = 0; i < Mesh.Indices.Num(); ++i)
        Tris[i] = (int32)Mesh.Indices[i];

    NewPMC->CreateMeshSection_LinearColor(
        0, Verts, Tris, Normals, UVs, LinearColors, Tangents,
        true, true);

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
        {
            PendingCollisionAdds.Add(Key);
        }
        else if (!bShouldHave && bHas && !bPendingRemove)
        {
            PendingCollisionRemoves.Add(Key);
        }
    }
}

void UVoxelTerrainComponent::ProcessCollisionQueue()
{
    int32 RemoveBudget = 2;
    int32 AddBudget = 1;

    while (PendingCollisionRemoves.Num() > 0 && RemoveBudget > 0)
    {
        FChunkKey Key = PendingCollisionRemoves.Pop();
        RemoveCollisionForChunk(Key);
        --RemoveBudget;
    }

    while (PendingCollisionAdds.Num() > 0 && AddBudget > 0)
    {
        FChunkKey Key = PendingCollisionAdds.Pop();
        --AddBudget;

        TSharedPtr<FVoxelMeshData, ESPMode::ThreadSafe>* MeshPtr =
            ChunkMeshes.Find(Key);
        if (!MeshPtr || !MeshPtr->IsValid()) continue;

        CreateCollisionForChunk(Key, **MeshPtr);
    }
}