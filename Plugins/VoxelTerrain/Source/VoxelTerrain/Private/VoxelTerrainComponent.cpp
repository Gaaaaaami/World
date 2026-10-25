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
#include "HAL/PlatformProcess.h"
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

    // ⭐ 预分配所有容器，避免加载期 rehash
    ChunkMeshes.Reserve(1024);
    ChunkLastActiveTime.Reserve(1024);
    ChunkCollisionKeepTime.Reserve(128);   // ⭐ 只给被编辑过的 chunk 用
    RequestedChunks.Reserve(1024);
    FileLoadedChunks.Reserve(1024);
    DirtyChunks.Reserve(64);
    ChunkCollisionProxies.Reserve(64);
    PendingCollisionAdds.Reserve(64);
    PendingCollisionRemoves.Reserve(64);
    PendingPushToSceneProxy.Reserve(64);
    PendingRebuilds.Reserve(64);
    ChunkVersions.Reserve(1024);
    ChunkModifications.Reserve(1024);

    // ⭐ 异步存档计数器
    PendingSaveCounter = MakeShared<FThreadSafeCounter>();

    // ⭐ Tick 周期 0.2s → 0.1s，配合 StreamTick 的分帧限量，加载更平滑
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

// ============================================================
// 存档系统
// ============================================================
FString UVoxelTerrainComponent::GetWorldSaveDir() const
{
    FString MapName = GetWorld()->GetMapName();
    MapName.RemoveFromStart(GetWorld()->StreamingLevelsPrefix);

    FString Dir = FPaths::ProjectSavedDir() / TEXT("VoxelWorlds") / MapName / TEXT("");
    if (!IFileManager::Get().DirectoryExists(*Dir))
    {
        IFileManager::Get().MakeDirectory(*Dir, true);
    }
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

void UVoxelTerrainComponent::SaveChunkModificationFileAsync(const FChunkKey& Key)
{
    if (Key.LOD != 0) return;
    if (!PendingSaveCounter.IsValid()) return;

    // ⭐ 主线程：持锁拷贝数据快照（受 DataLock 保护）
    TArray<uint8> SaveData;
    {
        FScopeLock Lock(&DataLock);
        FVoxelModificationMap* Mods = ChunkModifications.Find(Key);
        if (!Mods || Mods->Num() == 0) return;

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
    }

    if (SaveData.Num() == 0) return;

    // ⭐ 异步线程：写盘（不占主线程）
    const FString FilePath = GetChunkFilePath(Key);
    TSharedPtr<FThreadSafeCounter> Counter = PendingSaveCounter;
    Counter->Increment();

    Async(EAsyncExecution::ThreadPool,
        [SaveData = MoveTemp(SaveData), FilePath, Counter]()
        {
            FFileHelper::SaveArrayToFile(SaveData, *FilePath);
            Counter->Decrement();
        });
}

void UVoxelTerrainComponent::FlushAllPendingSaves()
{
    if (!PendingSaveCounter.IsValid()) return;

    // ⭐ 等待所有异步保存完成（10 秒超时保护，防止卡死）
    const double StartTime = FPlatformTime::Seconds();
    while (PendingSaveCounter->GetValue() > 0)
    {
        if (FPlatformTime::Seconds() - StartTime > 10.0)
        {
            UE_LOG(LogTemp, Warning,
                TEXT("VoxelTerrain: FlushAllPendingSaves 超时，仍有 %d 个保存未完成"),
                PendingSaveCounter->GetValue());
            break;
        }
        FPlatformProcess::Sleep(0.001f);
    }
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

    int32 MinX, MaxX, MinY, MaxY, MinZ, MaxZ;

    if (Key.LOD == 0) {
        MinX = Key.X - 1; MaxX = Key.X + 1;
        MinY = Key.Y - 1; MaxY = Key.Y + 1;
        MinZ = Key.Z - 1; MaxZ = Key.Z + 1;
    }
    else {
        int32 S = 1 << Key.LOD;
        MinX = (Key.X * S) - 1; MaxX = ((Key.X + 1) * S);
        MinY = (Key.Y * S) - 1; MaxY = ((Key.Y + 1) * S);
        MinZ = (Key.Z * S) - 1; MaxZ = ((Key.Z + 1) * S);
    }

    for (int32 x = MinX; x <= MaxX; ++x) {
        for (int32 y = MinY; y <= MaxY; ++y) {
            for (int32 z = MinZ; z <= MaxZ; ++z) {

                FChunkKey SubKey(x, y, z, 0);
                FScopeLock Lock(&DataLock);
                LoadChunkModificationFile(SubKey);
                if (const FVoxelModificationMap* Mods = ChunkModifications.Find(SubKey)) {
                    ResultMods.Append(*Mods);
                }
            }
        }
    }

    return ResultMods;
}

// ============================================================
// ⭐ 统一的 chunk 异步生成任务
// ============================================================
void UVoxelTerrainComponent::SpawnChunkAsyncTask(const FChunkKey& Key, int32 CurrentVersion)
{
    TWeakObjectPtr<UVoxelTerrainComponent> WeakThis(this);

    Async(EAsyncExecution::ThreadPool, [WeakThis, Key, CurrentVersion]()
        {
            FVoxelModificationMap ModsCopy;
            if (auto* Self = WeakThis.Get())
            {
                //FScopeLock Lock(&Self->DataLock);
                ModsCopy = Self->GatherModificationsForChunk(Key);
            }

            FVoxelGenerator Gen;
            FVoxelChunkData ChunkData;
            Gen.GenerateChunk(Key.X, Key.Y, Key.Z, Key.LOD, ModsCopy,
                ChunkData.Voxels.GetData());

            auto MeshData = MakeShared<FVoxelMeshData, ESPMode::ThreadSafe>();
            FTransvoxelMesher::GenerateRegular(
                ChunkData, VoxelConfig::VoxelSize, Key.LOD, *MeshData);

            // ⭐ 一个循环里同时完成：世界偏移 + UV 计算 + Bounds 累积
            const float S = (float)(1 << Key.LOD);
            const float LODChunkWorldSize =
                VoxelConfig::ChunkSize * VoxelConfig::VoxelSize * S;
            const FVector3f Offset(
                Key.X * LODChunkWorldSize,
                Key.Y * LODChunkWorldSize,
                Key.Z * LODChunkWorldSize);

            constexpr float UVScale = 400.f;
            const int32 NumVerts = MeshData->Positions.Num();
            MeshData->UVs.SetNumUninitialized(NumVerts);

            FBox Bounds(ForceInit);
            for (int32 i = 0; i < NumVerts; ++i)
            {
                FVector3f& P = MeshData->Positions[i];
                P += Offset;
                MeshData->UVs[i] = FVector2f(P.X, P.Y) / UVScale;
                Bounds += FVector(P);
            }
            MeshData->Bounds = Bounds;

            AsyncTask(ENamedThreads::GameThread,
                [WeakThis, Key, MeshData, CurrentVersion]()
                {
                    if (auto* Self = WeakThis.Get())
                        Self->OnChunkGenerated(Key, MeshData, CurrentVersion);
                });
        });
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

    // ============================================================
    // ⭐ 消化上一次编辑遗留的待重建 chunk（分帧）
    // ============================================================
    constexpr int32 MaxRebuildsPerTick = 6;
    if (PendingRebuilds.Num() > 0)
    {
        // 拷贝一份再迭代，避免 Async 完成时 Remove 造成迭代器失效
        TArray<FChunkKey> PendingList;
        PendingList.Reserve(PendingRebuilds.Num());
        for (const FChunkKey& Key : PendingRebuilds) PendingList.Add(Key);

        int32 Submitted = 0;
        for (const FChunkKey& Key : PendingList)
        {
            if (Submitted >= MaxRebuildsPerTick) break;
            if (RequestedChunks.Contains(Key)) continue;
            if (!ChunkMeshes.Contains(Key))
            {
                PendingRebuilds.Remove(Key);   // ⭐ 已卸载，不再需要重建
                continue;
            }
            RequestedChunks.Add(Key);
            const int32 CurrentVersion = ChunkVersions.FindRef(Key);
            SpawnChunkAsyncTask(Key, CurrentVersion);
            ++Submitted;
        }
    }

    // ============================================================
    // ⭐ 收集候选 chunk 并按距离排序，每 tick 限量提交
    // ============================================================
    struct FLoadCandidate
    {
        float     DistSq = 0.f;
        FChunkKey Key;
    };
    TArray<FLoadCandidate> Candidates;
    Candidates.Reserve(1024);

    const float BaseChunkSize = VoxelConfig::ChunkSize * VoxelConfig::VoxelSize;
    const FIntVector Base = WorldToBaseChunk(CamLoc);

    const int32 RadiusChunkXY = FMath::CeilToInt(LOD0RadiusXY / BaseChunkSize);
    const int32 RadiusChunkZ = FMath::CeilToInt(LOD0RadiusZ / BaseChunkSize);

    for (int32 dx = -RadiusChunkXY; dx <= RadiusChunkXY; ++dx)
    {
        for (int32 dy = -RadiusChunkXY; dy <= RadiusChunkXY; ++dy)
        {
            for (int32 dz = -RadiusChunkZ; dz <= RadiusChunkZ; ++dz)
            {
                FChunkKey Key(Base.X + dx, Base.Y + dy, Base.Z + dz, 0);

                // 已加载（含正在 rebuild 的情况）：只刷新活跃时间
                if (ChunkMeshes.Contains(Key))
                {
                    ChunkLastActiveTime.FindOrAdd(Key) = Now;
                    continue;
                }

                // 已在请求队列中
                if (RequestedChunks.Contains(Key)) continue;

                const FVector Center(
                    (Key.X + 0.5f) * BaseChunkSize,
                    (Key.Y + 0.5f) * BaseChunkSize,
                    (Key.Z + 0.5f) * BaseChunkSize);

                Candidates.Add({ (float)FVector::DistSquared(Center, CamLoc), Key });
            }
        }
    }

    // ⭐ 根据移动速度动态调整加载带宽
    const float SpeedSq = Pawn->GetVelocity().SizeSquared();
    int32 MaxRequestsPerTick = 8;
    if (SpeedSq > 4000000.f)   MaxRequestsPerTick = 16;   // > 2000 cm/s
    if (SpeedSq > 10000000.f)  MaxRequestsPerTick = 24;   // > 3162 cm/s（自由落体）

    if (Candidates.Num() > 0)
    {
        if (Candidates.Num() > MaxRequestsPerTick)
        {
            Candidates.Sort([](const FLoadCandidate& A, const FLoadCandidate& B)
                {
                    return A.DistSq < B.DistSq;
                });
        }

        const int32 Count = FMath::Min(Candidates.Num(), MaxRequestsPerTick);
        for (int32 i = 0; i < Count; ++i)
        {
            const FChunkKey& Key = Candidates[i].Key;
            RequestedChunks.Add(Key);
            const int32 CurrentVersion = ChunkVersions.FindRef(Key);
            SpawnChunkAsyncTask(Key, CurrentVersion);
        }
    }

    // ============================================================
    // 卸载
    // ============================================================
    const float UnloadRadiusXY = LOD0RadiusXY + BaseChunkSize;
    const float UnloadRadiusZ = LOD0RadiusZ + BaseChunkSize;

    TArray<FChunkKey> ToUnload;
    for (auto& Pair : ChunkMeshes)
    {
        const FChunkKey& Key = Pair.Key;
        if (Key.LOD != 0) continue;

        const FVector ChunkCenter(
            (Key.X + 0.5f) * BaseChunkSize,
            (Key.Y + 0.5f) * BaseChunkSize,
            (Key.Z + 0.5f) * BaseChunkSize);

        const FVector AbsDelta = (ChunkCenter - CamLoc).GetAbs();

        const bool bKeep =
            AbsDelta.X < UnloadRadiusXY &&
            AbsDelta.Y < UnloadRadiusXY &&
            AbsDelta.Z < UnloadRadiusZ;
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
        ChunkCollisionKeepTime.Remove(Key);   // ⭐
        RequestedChunks.Remove(Key);
        PendingPushToSceneProxy.Remove(Key);
        PendingRebuilds.Remove(Key);
        ChunkVersions.Remove(Key);
        RemoveCollisionForChunk(Key);

        // ⭐ 改成异步保存：主线程只拷贝数据快照，写盘丢到线程池
        bool bNeedsSave = false;
        {
            FScopeLock Lock(&DataLock);
            bNeedsSave = DirtyChunks.Contains(Key);
            if (bNeedsSave) DirtyChunks.Remove(Key);
        }
        if (bNeedsSave)
        {
            SaveChunkModificationFileAsync(Key);
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

    if (ToUnload.Num() > 0) bBoundsDirty = true;

    // ============================================================
    // 碰撞 & Bounds
    // ============================================================
    UpdateCollisions(CamLoc, Now);

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
    if (!Mesh.IsValid()) {
        RequestedChunks.Remove(Key);   // ⭐ 保护：失败也要清请求标记
        return;
    }
    if (Version != ChunkVersions.FindRef(Key)) {
        RequestedChunks.Remove(Key);   // ⭐ 保护：失败也要清请求标记
        return;
    }



    const bool bIsFirstTime = !ChunkMeshes.Contains(Key);

    ChunkMeshes.Add(Key, Mesh);
    ChunkLastActiveTime.Add(Key, FPlatformTime::Seconds());

    PendingRebuilds.Remove(Key);
    RequestedChunks.Remove(Key);

    // ⭐ Async 已预算好 Bounds，主线程只做 O(1) 合并
    if (bIsFirstTime && Mesh->Bounds.IsValid)
    {
        LocalBounds += Mesh->Bounds;
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
void UVoxelTerrainComponent::UpdateCollisions(const FVector& CamLoc, double Now)
{
    if (!bGenerateCollision) return;

    const float RadiusSquared = CollisionRadius * CollisionRadius;

    // ⭐ 检查玩家是否在快速下落
    float PlayerVelZ = 0.f;
    if (UWorld* World = GetWorld())
    {
        if (APlayerController* PC = World->GetFirstPlayerController())
        {
            if (APawn* Pawn = PC->GetPawnOrSpectator())
            {
                PlayerVelZ = Pawn->GetVelocity().Z;
            }
        }
    }
    const bool bFallingFast = PlayerVelZ < -1000.f;

    for (auto& Pair : ChunkMeshes)
    {
        const FChunkKey& Key = Pair.Key;
        if (Key.LOD != 0) continue;

        const float BaseChunkSize = VoxelConfig::ChunkSize * VoxelConfig::VoxelSize;
        const FVector ChunkCenter(
            (Key.X + 0.5f) * BaseChunkSize,
            (Key.Y + 0.5f) * BaseChunkSize,
            (Key.Z + 0.5f) * BaseChunkSize);

        const bool bInRange = FVector::DistSquared(ChunkCenter, CamLoc) < RadiusSquared;

        bool bKeepAlive = false;
        if (const double* KeepUntil = ChunkCollisionKeepTime.Find(Key))
        {
            bKeepAlive = (Now < *KeepUntil);
            if (!bKeepAlive) ChunkCollisionKeepTime.Remove(Key);
        }

        bool bShouldHave = bInRange || bKeepAlive;

        // ⭐ 快速下落时，跳过玩家上方 20m 外的碰撞创建（已有碰撞的保留）
        if (bShouldHave && bFallingFast && !bKeepAlive)
        {
            if (!ChunkCollisionProxies.Contains(Key))
            {
                const float AbovePlayer = ChunkCenter.Z - CamLoc.Z;
                if (AbovePlayer > 2000.f)
                {
                    bShouldHave = false;
                }
            }
        }

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
    // ⭐ 拿到玩家位置用于优先级排序
    FVector PlayerLoc = FVector::ZeroVector;
    float PlayerVelZ = 0.f;
    if (UWorld* World = GetWorld())
    {
        if (APlayerController* PC = World->GetFirstPlayerController())
        {
            if (APawn* Pawn = PC->GetPawnOrSpectator())
            {
                PlayerLoc = Pawn->GetActorLocation();
                PlayerVelZ = Pawn->GetVelocity().Z;
            }
        }
    }
    const bool bFallingFast = PlayerVelZ < -1000.f;

    const float BaseChunkSize = VoxelConfig::ChunkSize * VoxelConfig::VoxelSize;

    auto ChunkCenter = [BaseChunkSize](const FChunkKey& K) -> FVector
    {
        return FVector(
            (K.X + 0.5f) * BaseChunkSize,
            (K.Y + 0.5f) * BaseChunkSize,
            (K.Z + 0.5f) * BaseChunkSize);
    };

    int32 RemoveBudget = 2;
    int32 AddBudget = 3;   // ⭐ 2 → 3：单帧多处理 1 个，追赶下落速度

    while (PendingCollisionRemoves.Num() > 0 && RemoveBudget > 0)
    {
        FChunkKey Key = PendingCollisionRemoves.Pop();
        RemoveCollisionForChunk(Key);
        --RemoveBudget;
    }

    while (PendingCollisionAdds.Num() > 0 && AddBudget > 0)
    {
        // ⭐ 每次 Pop 前按"离玩家距离"排序，近的优先
        if (PendingCollisionAdds.Num() > 1)
        {
            PendingCollisionAdds.Sort([&ChunkCenter, PlayerLoc](const FChunkKey& A, const FChunkKey& B)
            {
                return FVector::DistSquared(ChunkCenter(A), PlayerLoc)
                     < FVector::DistSquared(ChunkCenter(B), PlayerLoc);
            });
        }

        const FChunkKey Key = PendingCollisionAdds.Pop();
        --AddBudget;

        // ⭐ 快速下落时，跳过玩家上方 20m 以外的 chunk（下一帧再处理）
        if (bFallingFast)
        {
            const float AbovePlayer = ChunkCenter(Key).Z - PlayerLoc.Z;
            if (AbovePlayer > 2000.f)
            {
                // 塞回队列尾部，等玩家停下来再处理
                PendingCollisionAdds.Insert(Key, 0);
                continue;
            }
        }

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
    // ⭐ 删除内部防抖：Actor 层的 EditCooldown (0.2s) 已足够节流

    const float VoxelSize = VoxelConfig::VoxelSize;
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
    const float BaseChunkSize = VoxelConfig::ChunkSize * VoxelConfig::VoxelSize;
    FVoxelGenerator Gen;

    // ⭐ 记录真正需要重建的 chunk
    TSet<FChunkKey> TouchedChunks;
    TouchedChunks.Reserve(16);

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
                        FMath::FloorToInt((x * VoxelSize) / BaseChunkSize),
                        FMath::FloorToInt((y * VoxelSize) / BaseChunkSize),
                        FMath::FloorToInt((z * VoxelSize) / BaseChunkSize)
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

                    // ⭐ 精确收集受影响 chunk：本体 + 仅触负边界的邻居
                    //    （FVoxelChunkData 相邻 chunk 只共享一条采样边，
                    //     体素的 Local==0 才影响负方向邻居，正方向邻居采样不到）
                    const int32 LocalX = x - ChunkCoord.X * VoxelConfig::ChunkSize;
                    const int32 LocalY = y - ChunkCoord.Y * VoxelConfig::ChunkSize;
                    const int32 LocalZ = z - ChunkCoord.Z * VoxelConfig::ChunkSize;

                    const bool bNegX = (LocalX == 0);
                    const bool bNegY = (LocalY == 0);
                    const bool bNegZ = (LocalZ == 0);

                    TouchedChunks.Add(LOD0Key);

                    if (bNegX)
                        TouchedChunks.Add(FChunkKey(ChunkCoord.X - 1, ChunkCoord.Y, ChunkCoord.Z, 0));
                    if (bNegY)
                        TouchedChunks.Add(FChunkKey(ChunkCoord.X, ChunkCoord.Y - 1, ChunkCoord.Z, 0));
                    if (bNegZ)
                        TouchedChunks.Add(FChunkKey(ChunkCoord.X, ChunkCoord.Y, ChunkCoord.Z - 1, 0));

                    if (bNegX && bNegY)
                        TouchedChunks.Add(FChunkKey(ChunkCoord.X - 1, ChunkCoord.Y - 1, ChunkCoord.Z, 0));
                    if (bNegX && bNegZ)
                        TouchedChunks.Add(FChunkKey(ChunkCoord.X - 1, ChunkCoord.Y, ChunkCoord.Z - 1, 0));
                    if (bNegY && bNegZ)
                        TouchedChunks.Add(FChunkKey(ChunkCoord.X, ChunkCoord.Y - 1, ChunkCoord.Z - 1, 0));
                    if (bNegX && bNegY && bNegZ)
                        TouchedChunks.Add(FChunkKey(ChunkCoord.X - 1, ChunkCoord.Y - 1, ChunkCoord.Z - 1, 0));
                }
            }
        }
    }

    if (TouchedChunks.Num() == 0) return;

    // ⭐ 被编辑的 chunk 保留碰撞（解决玩家远处建造后碰撞丢失）
    {
        const double KeepUntil = FPlatformTime::Seconds() + CollisionKeepDuration;
        for (const FChunkKey& Key : TouchedChunks)
        {
            ChunkCollisionKeepTime.Add(Key, KeepUntil);
        }
    }

    RebuildTouchedChunks(TouchedChunks);
}

void UVoxelTerrainComponent::RebuildTouchedChunks(const TSet<FChunkKey>& TouchedChunks)
{
    // ⭐ 每帧最多提交 6 个，其余留在 PendingRebuilds 由 StreamTick 消化
    constexpr int32 MaxRebuildPerTick = 6;
    int32 Submitted = 0;

    for (const FChunkKey& Key : TouchedChunks)
    {
        // 未加载的 chunk 不重建（下次进入视野时由 GatherModificationsForChunk 自然生成）
        if (!ChunkMeshes.Contains(Key)) continue;

        PendingRebuilds.Add(Key);
        RequestedChunks.Remove(Key);
        PendingPushToSceneProxy.Remove(Key);
        PendingCollisionAdds.Remove(Key);

        int32& Ver = ChunkVersions.FindOrAdd(Key);
        Ver++;

        if (Submitted < MaxRebuildPerTick)
        {
            RequestChunkImmediate(Key);
            ++Submitted;
        }
    }
}

void UVoxelTerrainComponent::RequestChunkImmediate(const FChunkKey& Key)
{
    if (ChunkMeshes.Contains(Key) && !PendingRebuilds.Contains(Key)) return;
    if (RequestedChunks.Contains(Key)) return;

    RequestedChunks.Add(Key);
    const int32 CurrentVersion = ChunkVersions.FindRef(Key);
    SpawnChunkAsyncTask(Key, CurrentVersion);
}

void UVoxelTerrainComponent::RebuildChunksAround(const FVector& WorldPos, float Radius)
{
    // ⭐ 保留函数体（不再被 ApplyEdit 调用），未来如果需要在超大半径场景下兜底可使用。
    //    实际 ApplyEdit 走 RebuildTouchedChunks 精确路径，1~5 个 chunk 重建。
    const float BaseChunkSize = VoxelConfig::ChunkSize * VoxelConfig::VoxelSize;

    const float Extent0 = Radius + BaseChunkSize;
    const FIntVector Min0 = WorldToBaseChunk(WorldPos - FVector(Extent0));
    const FIntVector Max0 = WorldToBaseChunk(WorldPos + FVector(Extent0));

    const float LOD1ChunkSize = BaseChunkSize * 2.f;
    const float Extent1 = Radius + LOD1ChunkSize;
    const FVector WorldPosMin1 = WorldPos - FVector(Extent1);
    const FVector WorldPosMax1 = WorldPos + FVector(Extent1);
    const FIntVector Min1(
        FMath::FloorToInt(WorldPosMin1.X / LOD1ChunkSize),
        FMath::FloorToInt(WorldPosMin1.Y / LOD1ChunkSize),
        FMath::FloorToInt(WorldPosMin1.Z / LOD1ChunkSize)
    );
    const FIntVector Max1(
        FMath::FloorToInt(WorldPosMax1.X / LOD1ChunkSize),
        FMath::FloorToInt(WorldPosMax1.Y / LOD1ChunkSize),
        FMath::FloorToInt(WorldPosMax1.Z / LOD1ChunkSize)
    );

    TArray<FChunkKey> ToRebuild;

    for (int32 x = Min0.X; x <= Max0.X; ++x) {
        for (int32 y = Min0.Y; y <= Max0.Y; ++y) {
            for (int32 z = Min0.Z; z <= Max0.Z; ++z) {
                FChunkKey Key(x, y, z, 0);
                if (ChunkMeshes.Contains(Key)) ToRebuild.Add(Key);
            }
        }
    }

    for (int32 x = Min1.X; x <= Max1.X; ++x) {
        for (int32 y = Min1.Y; y <= Max1.Y; ++y) {
            for (int32 z = Min1.Z; z <= Max1.Z; ++z) {
                FChunkKey Key(x, y, z, 1);
                if (ChunkMeshes.Contains(Key)) ToRebuild.Add(Key);
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
    // ⭐ 先等所有异步保存完成（否则清目录后可能又被异步写回）
    FlushAllPendingSaves();

    FScopeLock Lock(&DataLock);
    ChunkModifications.Empty();
    FileLoadedChunks.Empty();
    DirtyChunks.Empty();
    ChunkCollisionKeepTime.Empty();   // ⭐

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

    // ⭐ 收集所有脏 chunk 到本地列表，清空标记（避免重复保存），然后异步写盘
    TArray<FChunkKey> DirtyList;
    {
        FScopeLock Lock(&DataLock);
        DirtyList = DirtyChunks.Array();
        DirtyChunks.Empty();
    }

    for (const FChunkKey& Key : DirtyList)
    {
        SaveChunkModificationFileAsync(Key);
    }

    // ⭐ 等待所有异步保存完成（防止对象销毁后仍有写盘任务在跑）
    FlushAllPendingSaves();

    Super::EndPlay(Reason);
}