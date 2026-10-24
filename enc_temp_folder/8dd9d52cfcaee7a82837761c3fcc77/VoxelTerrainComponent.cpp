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
#include "Misc/Paths.h"

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
    ChunkModifications.Reserve(1024);

    TickHandle = FTSTicker::GetCoreTicker().AddTicker(
        FTickerDelegate::CreateWeakLambda(this, [this](float)
            {
                StreamTick();
                return true;
            }), 0.1f); // 加快一丢丢

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
    return FBoxSphereBounds(FBox(FVector(-1000000.f), FVector(1000000.f)));
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
    const float Size = VoxelConfig::ChunkWorldSize;
    return FIntVector(
        FMath::FloorToInt(WorldPos.X / Size),
        FMath::FloorToInt(WorldPos.Y / Size),
        FMath::FloorToInt(WorldPos.Z / Size));
}

// ============================================================
// 存档系统
// ============================================================
FString UVoxelTerrainComponent::GetWorldSaveDir() const
{
    FString MapName = GetWorld()->GetMapName();
    MapName.RemoveFromStart(GetWorld()->StreamingLevelsPrefix);
    FString Dir = FPaths::ProjectSavedDir() / TEXT("VoxelWorlds") / MapName / TEXT("");
    if (!IFileManager::Get().DirectoryExists(*Dir))
        IFileManager::Get().MakeDirectory(*Dir, true);
    return Dir;
}

FString UVoxelTerrainComponent::GetChunkFilePath(const FChunkKey& Key) const
{
    return GetWorldSaveDir() + FString::Printf(TEXT("Chunk_%d_%d_%d.vmod"), Key.X, Key.Y, Key.Z);
}

void UVoxelTerrainComponent::SaveChunkModificationFile(const FChunkKey& Key)
{
    if (Key.LOD != 0) return;
    FVoxelModificationMap* Mods = ChunkModifications.Find(Key);
    if (!Mods || Mods->Num() == 0) return;

    TArray<uint8> SaveData;
    FMemoryWriter Writer(SaveData, true);
    int32 Count = Mods->Num();
    Writer << Count;
    for (const auto& Pair : *Mods)
    {
        FIntVector Coord = Pair.Key;
        int8 Weight = Pair.Value;
        Writer << Coord;
        Writer << Weight;
    }
    FFileHelper::SaveArrayToFile(SaveData, *GetChunkFilePath(Key));
}

void UVoxelTerrainComponent::LoadChunkModificationFile(const FChunkKey& Key)
{
    if (Key.LOD != 0) return;
    if (FileLoadedChunks.Contains(Key)) return;

    TArray<uint8> LoadData;
    if (!FFileHelper::LoadFileToArray(LoadData, *GetChunkFilePath(Key)))
    {
        FileLoadedChunks.Add(Key);
        return;
    }

    FMemoryReader Reader(LoadData, true);
    int32 Count = 0;
    Reader << Count;

    FVoxelModificationMap& Mods = ChunkModifications.FindOrAdd(Key);
    for (int32 i = 0; i < Count; ++i)
    {
        FIntVector Coord;
        int8 Weight;
        Reader << Coord;
        Reader << Weight;
        Mods.Add(Coord, Weight);
    }
    FileLoadedChunks.Add(Key);
}

FVoxelModificationMap UVoxelTerrainComponent::GatherModificationsForChunk(const FChunkKey& Key)
{
    FVoxelModificationMap ResultMods;

    // 由于坐标系一致，LOD0 和 LOD1 完全共用相同坐标的修改数据
    FChunkKey LOD0Key(Key.X, Key.Y, Key.Z, 0);
    LoadChunkModificationFile(LOD0Key);
    if (const FVoxelModificationMap* Mods = ChunkModifications.Find(LOD0Key))
    {
        ResultMods = *Mods;
    }
    return ResultMods;
}

// ============================================================
// 流式加载
// ============================================================
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

            const int32 CurrentVersion = ChunkVersions.FindRef(Key);
            FVoxelModificationMap ModsCopy;
            {
                FScopeLock Lock(&DataLock);
                ModsCopy = GatherModificationsForChunk(Key);
            }

            Async(EAsyncExecution::ThreadPool, [WeakThis, Key, CurrentVersion, ModsCopy]()
                {
                    FVoxelGenerator Gen;
                    FVoxelChunkData ChunkData(Key.LOD);
                    Gen.GenerateChunk(Key.X, Key.Y, Key.Z, Key.LOD, ModsCopy, ChunkData.Voxels.GetData());

                    auto MeshData = MakeShared<FVoxelMeshData, ESPMode::ThreadSafe>();
                    FTransvoxelMesher::GenerateRegular(ChunkData, VoxelConfig::BaseVoxelSize, Key.LOD, *MeshData);

                    // ⭐ 尺寸统一，全部用 ChunkWorldSize
                    const float WorldSize = VoxelConfig::ChunkWorldSize;
                    const FVector3f Offset(
                        Key.X * WorldSize,
                        Key.Y * WorldSize,
                        Key.Z * WorldSize);

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

    const float WorldSize = VoxelConfig::ChunkWorldSize;
    const FIntVector Base = WorldToBaseChunk(CamLoc);

    const int32 RadiusChunkXY0 = FMath::CeilToInt(LOD0RadiusXY / WorldSize);
    const int32 RadiusChunkZ0 = FMath::CeilToInt(LOD0RadiusZ / WorldSize);

    const int32 RadiusChunkXY1 = FMath::CeilToInt(LOD1RadiusXY / WorldSize);
    const int32 RadiusChunkZ1 = FMath::CeilToInt(LOD1RadiusZ / WorldSize);

    // LOD0
    for (int32 dx = -RadiusChunkXY0; dx <= RadiusChunkXY0; ++dx)
    {
        for (int32 dy = -RadiusChunkXY0; dy <= RadiusChunkXY0; ++dy)
        {
            for (int32 dz = -RadiusChunkZ0; dz <= RadiusChunkZ0; ++dz)
            {
                FChunkKey Key(Base.X + dx, Base.Y + dy, Base.Z + dz, 0);
                RequestChunk(Key);
            }
        }
    }

    // LOD1
    for (int32 dx = -RadiusChunkXY1; dx <= RadiusChunkXY1; ++dx)
    {
        for (int32 dy = -RadiusChunkXY1; dy <= RadiusChunkXY1; ++dy)
        {
            for (int32 dz = -RadiusChunkZ1; dz <= RadiusChunkZ1; ++dz)
            {
                // ⭐ 绝对互斥：如果是 LOD0 的格子，绝对不给 LOD1
                if (FMath::Abs(dx) <= RadiusChunkXY0 &&
                    FMath::Abs(dy) <= RadiusChunkXY0 &&
                    FMath::Abs(dz) <= RadiusChunkZ0)
                {
                    continue;
                }
                FChunkKey Key(Base.X + dx, Base.Y + dy, Base.Z + dz, 1);
                RequestChunk(Key);
            }
        }
    }

    // 卸载
    TArray<FChunkKey> ToUnload;
    for (auto& Pair : ChunkMeshes)
    {
        const FChunkKey& Key = Pair.Key;

        const FVector ChunkCenter(
            (Key.X + 0.5f) * WorldSize,
            (Key.Y + 0.5f) * WorldSize,
            (Key.Z + 0.5f) * WorldSize);

        const FVector AbsDelta = (ChunkCenter - CamLoc).GetAbs();

        float UnloadRadiusXY, UnloadRadiusZ;
        if (Key.LOD == 0) {
            UnloadRadiusXY = LOD0RadiusXY + WorldSize;
            UnloadRadiusZ = LOD0RadiusZ + WorldSize;
        }
        else {
            UnloadRadiusXY = LOD1RadiusXY + WorldSize;
            UnloadRadiusZ = LOD1RadiusZ + WorldSize;
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

        if (DirtyChunks.Contains(Key))
        {
            FScopeLock Lock(&DataLock);
            SaveChunkModificationFile(Key);
            DirtyChunks.Remove(Key);
        }

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

    UpdateCollisions(CamLoc);
}

void UVoxelTerrainComponent::OnChunkGenerated(
    FChunkKey Key, TSharedPtr<FVoxelMeshData, ESPMode::ThreadSafe> Mesh, int32 Version)
{
    if (!Mesh.IsValid()) return;

    const int32 CurrentVersion = ChunkVersions.FindRef(Key);
    if (Version != CurrentVersion)
    {
        RequestedChunks.Remove(Key);
        return;
    }

    ChunkMeshes.Add(Key, Mesh);
    ChunkLastActiveTime.Add(Key, FPlatformTime::Seconds());

    PendingRebuilds.Remove(Key);
    RequestedChunks.Remove(Key);

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

    if (bGenerateCollision && Key.LOD == 0)
    {
        PendingCollisionAdds.Add(Key);
        PendingCollisionRemoves.Remove(Key);
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

// ============================================================
// 碰撞系统
// ============================================================
void UVoxelTerrainComponent::UpdateCollisions(const FVector& CamLoc)
{
    if (!bGenerateCollision) return;
    const float RadiusSquared = CollisionRadius * CollisionRadius;
    const float WorldSize = VoxelConfig::ChunkWorldSize;

    for (auto& Pair : ChunkMeshes)
    {
        const FChunkKey& Key = Pair.Key;
        if (Key.LOD != 0) continue;

        const FVector ChunkCenter(
            (Key.X + 0.5f) * WorldSize,
            (Key.Y + 0.5f) * WorldSize,
            (Key.Z + 0.5f) * WorldSize);

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
    int32 AddBudget = 2;

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

// ============================================================
// 编辑与重建
// ============================================================
void UVoxelTerrainComponent::ApplyEdit(const FVector& WorldPos, float Radius, bool bIsAdd)
{
    double Now = FPlatformTime::Seconds();
    if (Now - LastEditTime < 0.1) return;
    LastEditTime = Now;

    const float VoxelSize = VoxelConfig::BaseVoxelSize;
    const FVector GridPos = WorldPos / VoxelSize;

    const int32 RadiusInVoxels = FMath::CeilToInt(Radius / VoxelSize);
    const float SearchRadius = Radius + VoxelSize;
    const float SearchRadiusSq = SearchRadius * SearchRadius;

    const FIntVector MinGrid(FMath::FloorToInt(GridPos.X - RadiusInVoxels - 1),
        FMath::FloorToInt(GridPos.Y - RadiusInVoxels - 1),
        FMath::FloorToInt(GridPos.Z - RadiusInVoxels - 1));
    const FIntVector MaxGrid(FMath::FloorToInt(GridPos.X + RadiusInVoxels + 1),
        FMath::FloorToInt(GridPos.Y + RadiusInVoxels + 1),
        FMath::FloorToInt(GridPos.Z + RadiusInVoxels + 1));

    constexpr float Scale = 150.f;
    const float WorldSize = VoxelConfig::ChunkWorldSize;
    FVoxelGenerator Gen;

    {
        FScopeLock Lock(&DataLock);

        for (int32 x = MinGrid.X; x <= MaxGrid.X; ++x) {
            for (int32 y = MinGrid.Y; y <= MaxGrid.Y; ++y) {
                for (int32 z = MinGrid.Z; z <= MaxGrid.Z; ++z) {
                    const FVector VoxelWorldPos(x * VoxelSize, y * VoxelSize, z * VoxelSize);
                    const float DistSq = FVector::DistSquared(VoxelWorldPos, WorldPos);
                    if (DistSq > SearchRadiusSq) continue;

                    const float Dist = FMath::Sqrt(DistSq);
                    const float SDF = bIsAdd ? (Dist - Radius) : (Radius - Dist);
                    float W = FMath::Clamp(SDF / Scale, -1.f, 1.f);
                    int8 SDFWeight = (FVoxelWeight)FMath::RoundToInt(W * 127.f);

                    const FIntVector ChunkCoord(
                        FMath::FloorToInt((x * VoxelSize) / WorldSize),
                        FMath::FloorToInt((y * VoxelSize) / WorldSize),
                        FMath::FloorToInt((z * VoxelSize) / WorldSize)
                    );
                    FChunkKey LOD0Key(ChunkCoord.X, ChunkCoord.Y, ChunkCoord.Z, 0);

                    FVoxelModificationMap& Mods = ChunkModifications.FindOrAdd(LOD0Key);
                    FIntVector VoxelCoord(x, y, z);

                    int8 CurrentWeight;
                    if (const FVoxelWeight* Existing = Mods.Find(VoxelCoord))
                    {
                        CurrentWeight = *Existing;
                    }
                    else
                    {
                        float Density = -Gen.SampleDensity(VoxelWorldPos);
                        float BaseW = FMath::Clamp(Density / Scale, -1.f, 1.f);
                        CurrentWeight = (FVoxelWeight)FMath::RoundToInt(BaseW * 127.f);
                    }

                    int8 FinalWeight = bIsAdd
                        ? FMath::Min<int8>(CurrentWeight, SDFWeight)
                        : FMath::Max<int8>(CurrentWeight, SDFWeight);

                    Mods.Add(VoxelCoord, FinalWeight);
                    DirtyChunks.Add(LOD0Key);
                }
            }
        }
    }

    RebuildChunksAround(WorldPos, Radius);
}

void UVoxelTerrainComponent::RequestChunkImmediate(const FChunkKey& Key)
{
    if (ChunkMeshes.Contains(Key) && !PendingRebuilds.Contains(Key)) return;

    RequestedChunks.Remove(Key);
    if (RequestedChunks.Contains(Key)) return;
    RequestedChunks.Add(Key);

    const int32 CurrentVersion = ChunkVersions.FindRef(Key);

    FVoxelModificationMap ModsCopy;
    {
        FScopeLock Lock(&DataLock);
        ModsCopy = GatherModificationsForChunk(Key);
    }

    TWeakObjectPtr<UVoxelTerrainComponent> WeakThis(this);
    Async(EAsyncExecution::ThreadPool, [WeakThis, Key, CurrentVersion, ModsCopy]()
        {
            FVoxelGenerator Gen;
            FVoxelChunkData ChunkData(Key.LOD);
            Gen.GenerateChunk(Key.X, Key.Y, Key.Z, Key.LOD, ModsCopy, ChunkData.Voxels.GetData());

            auto MeshData = MakeShared<FVoxelMeshData, ESPMode::ThreadSafe>();
            FTransvoxelMesher::GenerateRegular(ChunkData, VoxelConfig::BaseVoxelSize, Key.LOD, *MeshData);

            const float WorldSize = VoxelConfig::ChunkWorldSize;
            const FVector3f Offset(
                Key.X * WorldSize,
                Key.Y * WorldSize,
                Key.Z * WorldSize);

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
}

void UVoxelTerrainComponent::RebuildChunksAround(const FVector& WorldPos, float Radius)
{
    const float WorldSize = VoxelConfig::ChunkWorldSize;
    const float Extent = Radius + WorldSize;
    const FIntVector Min = WorldToBaseChunk(WorldPos - FVector(Extent));
    const FIntVector Max = WorldToBaseChunk(WorldPos + FVector(Extent));

    TArray<FChunkKey> ToRebuild;

    for (int32 x = Min.X; x <= Max.X; ++x) {
        for (int32 y = Min.Y; y <= Max.Y; ++y) {
            for (int32 z = Min.Z; z <= Max.Z; ++z) {
                FChunkKey Key0(x, y, z, 0);
                FChunkKey Key1(x, y, z, 1);
                if (ChunkMeshes.Contains(Key0)) ToRebuild.Add(Key0);
                if (ChunkMeshes.Contains(Key1)) ToRebuild.Add(Key1);
            }
        }
    }

    for (const FChunkKey& Key : ToRebuild)
    {
        PendingRebuilds.Add(Key);
        RequestedChunks.Remove(Key);
        PendingPushToSceneProxy.Remove(Key);
        PendingCollisionAdds.Remove(Key);

        int32& Ver = ChunkVersions.FindOrAdd(Key);
        Ver++;

        RequestChunkImmediate(Key);
    }
}

void UVoxelTerrainComponent::ClearAllSaveData()
{
    FScopeLock Lock(&DataLock);
    ChunkModifications.Empty();
    FileLoadedChunks.Empty();
    DirtyChunks.Empty();

    FString Dir = GetWorldSaveDir();
    IFileManager::Get().DeleteDirectory(*Dir, false, true);

    UE_LOG(LogTemp, Warning, TEXT("VoxelTerrain: 存档已清除，目录: %s"), *Dir);

    TArray<FChunkKey> Keys;
    ChunkMeshes.GetKeys(Keys);

    for (const FChunkKey& Key : Keys)
    {
        ChunkMeshes.Remove(Key);
        RequestedChunks.Remove(Key);
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

    LocalBounds = FBox(ForceInit);
    bBoundsDirty = true;
    UpdateBounds();
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

    FScopeLock Lock(&DataLock);
    for (const FChunkKey& Key : DirtyChunks)
    {
        SaveChunkModificationFile(Key);
    }
    DirtyChunks.Empty();

    Super::EndPlay(Reason);
}