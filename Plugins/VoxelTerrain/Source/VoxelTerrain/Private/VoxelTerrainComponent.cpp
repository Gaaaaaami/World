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
#include "HAL/FileManager.h"
#include "Serialization/MemoryWriter.h"
#include "Serialization/MemoryReader.h"
#include "Misc/FileHelper.h"

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
    PendingPushToSceneProxy.Reserve(64);
    PendingRebuilds.Reserve(64);
    ChunkVersions.Reserve(1024);

    TickHandle = FTSTicker::GetCoreTicker().AddTicker(
        FTickerDelegate::CreateWeakLambda(this, [this](float)
            {
                StreamTick();
                return true;
            }), 0.2f);

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
    if (LocalBounds.IsValid)
    {
        return FBoxSphereBounds(LocalBounds);
    }
    return FBoxSphereBounds(FBox(FVector(-100), FVector(100)));
}

void UVoxelTerrainComponent::GetUsedMaterials(
    TArray<UMaterialInterface*>& OutMaterials, bool bGetDebugMaterials) const
{
    if (TerrainMaterial)
        OutMaterials.Add(TerrainMaterial);
    else if (UMaterial* DefaultMat = UMaterial::GetDefaultMaterial(MD_Surface))
        OutMaterials.Add(DefaultMat);
}

FIntVector UVoxelTerrainComponent::WorldToBaseChunk(const FVector& WorldPos) const
{
    const float BaseChunkSize = VoxelConfig::ChunkSize * VoxelConfig::VoxelSize;
    return FIntVector(
        FMath::FloorToInt(WorldPos.X / BaseChunkSize),
        FMath::FloorToInt(WorldPos.Y / BaseChunkSize),
        FMath::FloorToInt(WorldPos.Z / BaseChunkSize));
}

void UVoxelTerrainComponent::StreamTick()
{
    UWorld* World = GetWorld();
    if (!World) return;

    APlayerController* PC = World->GetFirstPlayerController();
    if (!PC) return;

    APawn* Pawn = PC->GetPawnOrSpectator();
    if (!Pawn) return;

    const FVector CamLoc = Pawn->GetActorLocation();
    const double Now = FPlatformTime::Seconds();

    FlushPendingPushToSceneProxy();

    TWeakObjectPtr<UVoxelTerrainComponent> WeakThis(this);

    auto RequestChunk = [this, WeakThis, Now](const FChunkKey& Key)
        {
            if (ChunkMeshes.Contains(Key) && !PendingRebuilds.Contains(Key))
            {
                ChunkLastActiveTime.Add(Key, Now);
                return;
            }
            if (RequestedChunks.Contains(Key)) return;
            RequestedChunks.Add(Key);

            FVoxelModificationMap ModsCopy = TerrainModifications;
            const int32 CurrentVersion = ChunkVersions.FindRef(Key);

            Async(EAsyncExecution::ThreadPool, [WeakThis, Key, ModsCopy, CurrentVersion]()
                {
                    FVoxelGenerator Gen;
                    FVoxelChunkData ChunkData;
                    Gen.GenerateChunk(Key.X, Key.Y, Key.Z, Key.LOD, ModsCopy, ChunkData.Voxels.GetData());

                    auto MeshData = MakeShared<FVoxelMeshData, ESPMode::ThreadSafe>();
                    FTransvoxelMesher::GenerateRegular(ChunkData, VoxelConfig::VoxelSize, Key.LOD, *MeshData);

                    const float S = (float)(1 << Key.LOD);
                    const float LODChunkWorldSize = VoxelConfig::ChunkSize * VoxelConfig::VoxelSize * S;
                    const FVector3f Offset(
                        Key.X * LODChunkWorldSize,
                        Key.Y * LODChunkWorldSize,
                        Key.Z * LODChunkWorldSize);

                    for (FVector3f& P : MeshData->Positions)
                    {
                        P += Offset;
                    }

                    const float UVScale = 400.f;
                    MeshData->UVs.SetNumUninitialized(MeshData->Positions.Num());
                    for (int32 i = 0; i < MeshData->Positions.Num(); ++i)
                    {
                        MeshData->UVs[i] = FVector2f(MeshData->Positions[i].X, MeshData->Positions[i].Y) / UVScale;
                    }

                    AsyncTask(ENamedThreads::GameThread, [WeakThis, Key, MeshData, CurrentVersion]()
                        {
                            if (auto* Self = WeakThis.Get())
                                Self->OnChunkGenerated(Key, MeshData, CurrentVersion);
                        });
                });
        };

    const FIntVector Base = WorldToBaseChunk(CamLoc);
    const float BaseChunkSize = VoxelConfig::ChunkSize * VoxelConfig::VoxelSize;

    for (int32 dx = -LOD0RadiusXY; dx <= LOD0RadiusXY; ++dx)
    {
        for (int32 dy = -LOD0RadiusXY; dy <= LOD0RadiusXY; ++dy)
        {
            for (int32 dz = -LOD0RadiusZ; dz <= LOD0RadiusZ; ++dz)
            {
                FChunkKey Key(Base.X + dx, Base.Y + dy, Base.Z + dz, 0);
                RequestChunk(Key);
            }
        }
    }

    const int32 B1X = Base.X >> 1;
    const int32 B1Y = Base.Y >> 1;
    const int32 B1Z = Base.Z >> 1;

    const float LOD1ChunkSize = BaseChunkSize * 2.f;
    const float LOD0WorldRadiusXY = (LOD0RadiusXY + 0.5f) * BaseChunkSize;
    const float LOD0WorldRadiusZ = (LOD0RadiusZ + 0.5f) * BaseChunkSize;

    for (int32 dx = -LOD1RadiusXY; dx <= LOD1RadiusXY; ++dx)
    {
        for (int32 dy = -LOD1RadiusXY; dy <= LOD1RadiusXY; ++dy)
        {
            for (int32 dz = -LOD1RadiusZ; dz <= LOD1RadiusZ; ++dz)
            {
                const int32 X = B1X + dx;
                const int32 Y = B1Y + dy;
                const int32 Z = B1Z + dz;

                const FVector ChunkCenter(
                    (X + 0.5f) * LOD1ChunkSize,
                    (Y + 0.5f) * LOD1ChunkSize,
                    (Z + 0.5f) * LOD1ChunkSize);

                const FVector Delta = ChunkCenter - CamLoc;
                const bool bInsideLOD0 =
                    FMath::Abs(Delta.X) < LOD0WorldRadiusXY &&
                    FMath::Abs(Delta.Y) < LOD0WorldRadiusXY &&
                    FMath::Abs(Delta.Z) < LOD0WorldRadiusZ;

                if (bInsideLOD0) continue;

                FChunkKey Key(X, Y, Z, 1);
                RequestChunk(Key);
            }
        }
    }

    TArray<FChunkKey> ToUnload;
    for (auto& Pair : ChunkMeshes)
    {
        const FChunkKey& Key = Pair.Key;
        if (PendingRebuilds.Contains(Key)) continue;

        const float S = (float)(1 << Key.LOD);
        const float ChunkSize = BaseChunkSize * S;

        const FVector ChunkCenter(
            (Key.X + 0.5f) * ChunkSize,
            (Key.Y + 0.5f) * ChunkSize,
            (Key.Z + 0.5f) * ChunkSize);

        const FVector AbsDelta = (ChunkCenter - CamLoc).GetAbs();

        float UnloadRadiusXY, UnloadRadiusZ;
        if (Key.LOD == 0) {
            UnloadRadiusXY = (LOD0RadiusXY + 1.f) * BaseChunkSize;
            UnloadRadiusZ = (LOD0RadiusZ + 1.f) * BaseChunkSize;
        }
        else {
            UnloadRadiusXY = (LOD1RadiusXY + 1.f) * LOD1ChunkSize;
            UnloadRadiusZ = (LOD1RadiusZ + 1.f) * LOD1ChunkSize;
        }

        const bool bKeep = AbsDelta.X < UnloadRadiusXY && AbsDelta.Y < UnloadRadiusXY && AbsDelta.Z < UnloadRadiusZ;
        if (bKeep) continue;

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
        PendingPushToSceneProxy.Remove(Key);
        PendingRebuilds.Remove(Key);
        ChunkVersions.Remove(Key);
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
    FChunkKey Key, TSharedPtr<FVoxelMeshData, ESPMode::ThreadSafe> Mesh, int32 Version)
{
    if (!Mesh.IsValid()) return;

    const bool bIsFirstTime = !ChunkMeshes.Contains(Key);

    ChunkMeshes.Add(Key, Mesh);
    ChunkLastActiveTime.Add(Key, FPlatformTime::Seconds());

    PendingRebuilds.Remove(Key);
    RequestedChunks.Remove(Key);

    if (bIsFirstTime)
    {
        for (const FVector3f& P : Mesh->Positions)
            LocalBounds += FVector(P);
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
    else
    {
        PendingPushToSceneProxy.Add(Key);
    }
}

void UVoxelTerrainComponent::FlushPendingPushToSceneProxy()
{
    if (PendingPushToSceneProxy.Num() == 0) return;

    FVoxelSceneProxy* Proxy = static_cast<FVoxelSceneProxy*>(SceneProxy);
    if (!Proxy) return;

    for (const FChunkKey& Key : PendingPushToSceneProxy)
    {
        if (auto* MeshPtr = ChunkMeshes.Find(Key))
        {
            TSharedPtr<FVoxelMeshData, ESPMode::ThreadSafe> Mesh = *MeshPtr;
            ENQUEUE_RENDER_COMMAND(AddVoxelChunk)(
                [Proxy, Key, Mesh](FRHICommandListImmediate& RHICmdList)
                {
                    Proxy->AddChunk_RenderThread(RHICmdList, Key, Mesh);
                });
        }
    }
    PendingPushToSceneProxy.Empty();
}

void UVoxelTerrainComponent::UpdateCollisions(const FVector& CamLoc)
{
    if (!bGenerateCollision) return;

    const float BaseChunkSize = VoxelConfig::ChunkSize * VoxelConfig::VoxelSize;
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

        const bool bShouldHave = FVector::DistSquared(ChunkCenter, CamLoc) < RadiusSquared;
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

        TSharedPtr<FVoxelMeshData, ESPMode::ThreadSafe>* MeshPtr = ChunkMeshes.Find(Key);
        if (!MeshPtr || !MeshPtr->IsValid()) continue;

        CreateCollisionForChunk(Key, **MeshPtr);
    }
}

void UVoxelTerrainComponent::CreateCollisionForChunk(const FChunkKey& Key, const FVoxelMeshData& Mesh)
{
    if (Mesh.Positions.Num() == 0 || Mesh.Indices.Num() == 0) return;

    if (ChunkCollisionProxies.Contains(Key))
    {
        RemoveCollisionForChunk(Key);
    }

    UProceduralMeshComponent* NewPMC = NewObject<UProceduralMeshComponent>(this, NAME_None, RF_Transient);
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
    TArray<FVector> Verts;
    TArray<int32> Tris;
    TArray<FVector> Normals;
    TArray<FVector2D> UVs;
    TArray<FLinearColor> Colors;
    TArray<FProcMeshTangent> Tangents;

    Verts.SetNumUninitialized(NumVerts);
    Normals.SetNumUninitialized(NumVerts);
    UVs.SetNumUninitialized(NumVerts);
    Colors.SetNumUninitialized(NumVerts);
    Tangents.SetNumUninitialized(NumVerts);

    for (int32 i = 0; i < NumVerts; ++i)
    {
        Verts[i] = FVector(Mesh.Positions[i]);
        Normals[i] = Mesh.Normals.IsValidIndex(i) ? FVector(Mesh.Normals[i]) : FVector(0, 0, 1);
        UVs[i] = Mesh.UVs.IsValidIndex(i) ? FVector2D(Mesh.UVs[i]) : FVector2D::ZeroVector;
        FColor C = Mesh.Colors.IsValidIndex(i) ? Mesh.Colors[i] : FColor::White;
        Colors[i] = FLinearColor(C);
        Tangents[i] = FProcMeshTangent(1, 0, 0);
    }

    Tris.SetNumUninitialized(Mesh.Indices.Num());
    for (int32 i = 0; i < Mesh.Indices.Num(); ++i)
        Tris[i] = (int32)Mesh.Indices[i];

    NewPMC->CreateMeshSection_LinearColor(0, Verts, Tris, Normals, UVs, Colors, Tangents, true, true);
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
        UProceduralMeshComponent* PMC = Pair.Value.Get();
        if (IsValid(PMC))
        {
            PMC->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            PMC->ClearAllMeshSections();
            PMC->DestroyComponent();
        }
    }
    ChunkCollisionProxies.Empty();
    PendingCollisionAdds.Empty();
    PendingCollisionRemoves.Empty();
}

void UVoxelTerrainComponent::ApplyEdit(const FVector& WorldPos, float Radius, bool bIsAdd)
{
    const float VoxelSize = VoxelConfig::VoxelSize;
    const FVector GridPos = WorldPos / VoxelSize;

    const int32 RadiusInVoxels = FMath::CeilToInt(Radius / VoxelSize);
    const float SearchRadius = Radius + VoxelSize; // 向外多扩一圈，用于平滑过渡
    const float SearchRadiusSq = SearchRadius * SearchRadius;

    // 计算出受影响的 AABB 范围
    const FIntVector MinGrid(FMath::FloorToInt(GridPos.X - RadiusInVoxels - 1),
        FMath::FloorToInt(GridPos.Y - RadiusInVoxels - 1),
        FMath::FloorToInt(GridPos.Z - RadiusInVoxels - 1));
    const FIntVector MaxGrid(FMath::FloorToInt(GridPos.X + RadiusInVoxels + 1),
        FMath::FloorToInt(GridPos.Y + RadiusInVoxels + 1),
        FMath::FloorToInt(GridPos.Z + RadiusInVoxels + 1));

    constexpr float Scale = 150.f;

    // 用于获取原始地形权重（如果没有被修改过）
    FVoxelGenerator Gen;

    for (int32 x = MinGrid.X; x <= MaxGrid.X; ++x) {
        for (int32 y = MinGrid.Y; y <= MaxGrid.Y; ++y) {
            for (int32 z = MinGrid.Z; z <= MaxGrid.Z; ++z) {

                const FVector VoxelWorldPos(x * VoxelSize, y * VoxelSize, z * VoxelSize);
                const float DistSq = FVector::DistSquared(VoxelWorldPos, WorldPos);

                if (DistSq > SearchRadiusSq) continue;

                const float Dist = FMath::Sqrt(DistSq);

                // 1. 计算球形 SDF 权重
                // 破坏 (bIsAdd = false)：球心是空气(正)，外部是实心(负)。
                // 建造 (bIsAdd = true)：球心是实心(负)，外部是空气(正)。
                const float SDF = bIsAdd ? (Dist - Radius) : (Radius - Dist);
                float W = FMath::Clamp(SDF / Scale, -1.f, 1.f);
                int8 SDFWeight = (FVoxelWeight)FMath::RoundToInt(W * 127.f);

                // 2. 获取该体素当前的权重（已被修改过就取字典，否则取原始地形）
                int8 CurrentWeight;
                FIntVector VoxelCoord(x, y, z);

                if (const FVoxelWeight* Existing = TerrainModifications.Find(VoxelCoord))
                {
                    CurrentWeight = *Existing;
                }
                else
                {
                    float Density = -Gen.SampleDensity(VoxelWorldPos);
                    float BaseW = FMath::Clamp(Density / Scale, -1.f, 1.f);
                    CurrentWeight = (FVoxelWeight)FMath::RoundToInt(BaseW * 127.f);
                }

                // 3. SDF 布尔运算
                // 挖坑：取 Max（只要有空气就是空气）
                // 建造：取 Min（只要有实心就是实心）
                int8 FinalWeight = bIsAdd
                    ? FMath::Min<int8>(CurrentWeight, SDFWeight)
                    : FMath::Max<int8>(CurrentWeight, SDFWeight);

                // 4. 存回字典
                TerrainModifications.Add(VoxelCoord, FinalWeight);
            }
        }
    }

    // 触发重建
    RebuildChunksAround(WorldPos, Radius);
}

void UVoxelTerrainComponent::RebuildChunksAround(const FVector& WorldPos, float Radius)
{
    const float BaseChunkSize = VoxelConfig::ChunkSize * VoxelConfig::VoxelSize;
    const float Extent = Radius + BaseChunkSize;
    const FIntVector MinChunk = WorldToBaseChunk(WorldPos - FVector(Extent));
    const FIntVector MaxChunk = WorldToBaseChunk(WorldPos + FVector(Extent));

    TArray<FChunkKey> ToRebuild;

    for (int32 x = MinChunk.X; x <= MaxChunk.X; ++x) {
        for (int32 y = MinChunk.Y; y <= MaxChunk.Y; ++y) {
            for (int32 z = MinChunk.Z; z <= MaxChunk.Z; ++z) {
                FChunkKey Key(x, y, z, 0);
                if (ChunkMeshes.Contains(Key)) {
                    ToRebuild.Add(Key);
                }
            }
        }
    }

    for (const FChunkKey& Key : ToRebuild)
    {
        PendingRebuilds.Add(Key);

        // ⭐ 强制清理所有状态，确保立即发起重新请求
        RequestedChunks.Remove(Key);
        PendingPushToSceneProxy.Remove(Key);
        PendingCollisionAdds.Remove(Key);

        // ⭐ 如果已经有碰撞体了，先删掉，防止玩家利用旧碰撞体导致射线打偏
        RemoveCollisionForChunk(Key);

        int32& Ver = ChunkVersions.FindOrAdd(Key);
        Ver++;
    }
}

void UVoxelTerrainComponent::SaveModifications(const FString& FilePath)
{
    TArray<uint8> SaveData;
    FMemoryWriter Writer(SaveData, true);

    int32 Count = TerrainModifications.Num();
    Writer << Count;

    for (const auto& Pair : TerrainModifications)
    {
        FIntVector Coord = Pair.Key;
        int8 Weight = Pair.Value;
        Writer << Coord;
        Writer << Weight;
    }

    if (FFileHelper::SaveArrayToFile(SaveData, *FilePath))
    {
        //UE_LOG(LogTemp, Log, TEXT("VoxelTerrain: Saved %d modifications to %s"), Count, *FilePath);
    }
}

void UVoxelTerrainComponent::LoadModifications(const FString& FilePath)
{
    TArray<uint8> LoadData;
    if (!FFileHelper::LoadFileToArray(LoadData, *FilePath)) return;

    FMemoryReader Reader(LoadData, true);
    int32 Count = 0;
    Reader << Count;

    TerrainModifications.Empty(Count);
    for (int32 i = 0; i < Count; ++i)
    {
        FIntVector Coord;
        int8 Weight;
        Reader << Coord;
        Reader << Weight;
        TerrainModifications.Add(Coord, Weight);
    }

    //UE_LOG(LogTemp, Log, TEXT("VoxelTerrain: Loaded %d modifications from %s"), Count, *FilePath);
    RebuildChunksAround(GetComponentLocation(), 50000.f);
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