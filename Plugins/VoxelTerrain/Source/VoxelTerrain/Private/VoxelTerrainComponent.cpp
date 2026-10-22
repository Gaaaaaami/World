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
    PendingGPUAdds.Reserve(256);
    PendingGPURemoves.Reserve(256);
    ChunkCollisionProxies.Reserve(64);
    PendingCollisionAdds.Reserve(64);
    PendingCollisionRemoves.Reserve(64);

    TickHandle = FTSTicker::GetCoreTicker().AddTicker(
        FTickerDelegate::CreateWeakLambda(this, [this](float)
            {
                StreamTick();
                return true;
            }), 0.1f);

    RenderTickHandle = FTSTicker::GetCoreTicker().AddTicker(
        FTickerDelegate::CreateWeakLambda(this, [this](float)
            {
                FlushPendingGPUCommands();
                return true;
            }), 0.f);

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
    // ⭐ 主线程数据结构更新保持原子：一次切换内旧 Key 全部移除、新 Key 全部落位。
    //   这保证 ChunkMeshes / StagingMeshes / ChunkLastActiveTime / 碰撞代理
    //   的状态与原来完全一致。
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

    // ⭐ 关键修改：不再一次性 ENQUEUE 到渲染线程。
    //   - Add 走 PendingGPUAdds 头部（LOD 切换优先级高于普通流式加载）
    //   - Remove 追加 PendingGPURemoves 尾部
    //   - FlushPendingGPUCommands 内先 Add 再 Remove，保证视觉上"先补再拆"
    // 代价：切换瞬间 1~2 帧内新旧 chunk 可能共存（同一片等值面，视觉难以察觉），
    //       但消除了单帧集中上传 8 个 chunk 造成的 2~5ms 掉帧。
    TArray<FPendingAdd> NewAdds;
    NewAdds.Reserve(NewChunks.Num());
    for (const auto& Item : NewChunks)
    {
        if (!Item.Value.IsValid()) continue;
        if (Item.Value->Positions.Num() == 0 || Item.Value->Indices.Num() == 0) continue;

        FPendingAdd PA;
        PA.Key = Item.Key;
        PA.Mesh = Item.Value;
        NewAdds.Add(PA);
    }
    if (NewAdds.Num() > 0)
    {
        PendingGPUAdds.Insert(MoveTemp(NewAdds), 0);
    }

    for (const FChunkKey& K : OldKeys)
    {
        PendingGPURemoves.Add(K);
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

// ============================================================
// 分帧 GPU 命令派发：从队列里挑离玩家最近的先上传
// ============================================================
void UVoxelTerrainComponent::FlushPendingGPUCommands()
{
    if (PendingGPUAdds.Num() == 0 && PendingGPURemoves.Num() == 0)
        return;

    FVoxelSceneProxy* Proxy = static_cast<FVoxelSceneProxy*>(SceneProxy);
    if (!Proxy)
    {
        PendingGPUAdds.Empty();
        PendingGPURemoves.Empty();
        return;
    }

    // 玩家位置（用于距离排序）
    FVector CamLoc = GetComponentLocation();
    if (UWorld* World = GetWorld())
    {
        if (APlayerController* PC = World->GetFirstPlayerController())
        {
            if (APawn* Pawn = PC->GetPawnOrSpectator())
                CamLoc = Pawn->GetActorLocation();
        }
    }

    const float BaseChunkSize =
        VoxelConfig::ChunkSize * VoxelConfig::VoxelSize;

    auto DistSqOf = [&CamLoc, BaseChunkSize](const FChunkKey& K)
        {
            const float CS = BaseChunkSize * (float)(1 << K.LOD);
            const FVector Center(
                (K.X + 0.5f) * CS, (K.Y + 0.5f) * CS, (K.Z + 0.5f) * CS);
            return FVector::DistSquared(Center, CamLoc);
        };

    const int32 Budget = FMath::Max(1, MaxGPUCommandsPerFrame);
    int32 Processed = 0;

    // ⭐ 优先处理 Add：每次从队列里挑离玩家最近的
    while (PendingGPUAdds.Num() > 0 && Processed < Budget)
    {
        int32 BestIdx = 0;
        float BestDistSq = DistSqOf(PendingGPUAdds[0].Key);
        for (int32 i = 1; i < PendingGPUAdds.Num(); ++i)
        {
            const float D = DistSqOf(PendingGPUAdds[i].Key);
            if (D < BestDistSq) { BestDistSq = D; BestIdx = i; }
        }

        FPendingAdd Item = PendingGPUAdds[BestIdx];
        PendingGPUAdds.RemoveAtSwap(BestIdx);

        // chunk 在排队期间可能已被卸载/被反向 LOD 切换覆盖，跳过
        if (!ChunkMeshes.Contains(Item.Key)) continue;

        auto Key = Item.Key;
        auto Mesh = Item.Mesh;
        ENQUEUE_RENDER_COMMAND(AddVoxelChunk)(
            [Proxy, Key, Mesh](FRHICommandListImmediate& RHICmdList)
            {
                Proxy->AddChunk_RenderThread(RHICmdList, Key, Mesh);
            });
        ++Processed;
    }

    // 剩余预算处理 Remove
    while (PendingGPURemoves.Num() > 0 && Processed < Budget)
    {
        FChunkKey Key = PendingGPURemoves[0];
        PendingGPURemoves.RemoveAt(0);

        // ⭐ Key 若因玩家快速来回移动而重新回到 ChunkMeshes（或在 PendingGPUAdds 中等待），
        //   说明它应该继续存在，不应移除。
        if (ChunkMeshes.Contains(Key)) continue;

        ENQUEUE_RENDER_COMMAND(RemoveVoxelChunk)(
            [Proxy, Key](FRHICommandListImmediate& RHICmdList)
            {
                Proxy->RemoveChunk_RenderThread(RHICmdList, Key);
            });
        ++Processed;
    }
}

// ============================================================
// 主循环
// ============================================================
void UVoxelTerrainComponent::StreamTick()
{
    UWorld* World = GetWorld();
    if (!World) return;

    FVector CamLoc = GetComponentLocation();
    FVector CamVel = FVector::ZeroVector;
    if (APlayerController* PC = World->GetFirstPlayerController())
    {
        if (APawn* Pawn = PC->GetPawnOrSpectator())
        {
            CamLoc = Pawn->GetActorLocation();
            CamVel = Pawn->GetVelocity();
        }
    }

    const double Now = FPlatformTime::Seconds();
    TWeakObjectPtr<UVoxelTerrainComponent> WeakThis(this);

    // ---------- 1) 期望 chunk 集合（缓存） ----------
    // ⭐ 阈值从 100（1m）提高到 1/4 chunk（8m），避免玩家跳跃/坠落时每 tick 全量重算。
    //   业务语义不变：仍然"相机移动即重算"，只是把重算频率降到更合理的粒度。
    constexpr float DesiredCacheThreshold =
        VoxelConfig::ChunkSize * VoxelConfig::VoxelSize * 0.25f;

    if (!CamLoc.Equals(LastDesiredCamLoc, DesiredCacheThreshold))
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

    // ---------- 3) 加载：按距离 + Z 加权 + 移动方向排序 ----------
    TArray<FChunkKey> SortedDesired;
    SortedDesired.Reserve(DesiredChunks.Num());
    for (const FChunkKey& K : DesiredChunks) SortedDesired.Add(K);

    FVector CamDir = FVector::ZeroVector;
    if (!CamVel.IsNearlyZero())
        CamDir = CamVel.GetSafeNormal();

    SortedDesired.Sort([&CamLoc, &CamDir](const FChunkKey& A, const FChunkKey& B)
        {
            auto Score = [&CamLoc, &CamDir](const FChunkKey& K)
                {
                    const float CS = VoxelConfig::ChunkSize
                        * VoxelConfig::VoxelSize * (float)(1 << K.LOD);
                    const FVector Center(
                        (K.X + 0.5f) * CS, (K.Y + 0.5f) * CS, (K.Z + 0.5f) * CS);
                    const FVector D = Center - CamLoc;

                    // Z 方向加权 1.5 倍（垂直移动时，下/上方 chunk 更优先）
                    float Sq = D.X * D.X + D.Y * D.Y + 2.25f * D.Z * D.Z;

                    // 移动方向预判：朝移动方向的 chunk 距离打折
                    if (!CamDir.IsNearlyZero() && Sq > 1.f)
                    {
                        const FVector DN = D.GetSafeNormal();
                        const float Dot = FVector::DotProduct(DN, CamDir);
                        if (Dot > 0.f)
                            Sq *= (1.f - 0.3f * Dot);
                    }
                    return Sq;
                };
            return Score(A) < Score(B);
        });

    int32 NewRequests = 0;
    const int32 MaxInFlight = FMath::Max(1, MaxInFlightRequests);
    for (const FChunkKey& Key : SortedDesired)
    {
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


                // 2. 直接根据世界坐标的 X/Y 算 UV（极度平滑）
                const float UVScale = 400.f; // 如果觉得纹理小，可以改大（如 600, 800）
                MeshData->UVs.SetNumUninitialized(MeshData->Positions.Num());

                for (int32 i = 0; i < MeshData->Positions.Num(); ++i)
                {
                    MeshData->UVs[i] = FVector2f(MeshData->Positions[i].X, MeshData->Positions[i].Y) / UVScale;
                }
 

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
            TEXT("VoxelTerrain: +%d chunks (showing=%d, staging=%d, inFlight=%d, pendingGPU=%d, desired=%d)"),
            NewRequests, ChunkMeshes.Num(), StagingMeshes.Num(),
            RequestedChunks.Num(), PendingGPUAdds.Num(), DesiredChunks.Num());
    }

    // ---------- 4) LOD 原子切换（内部改为分帧派发） ----------
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

        // 卸载走分帧队列
        PendingGPURemoves.Add(Key);
    }

    if (ToUnload.Num() > 0) bBoundsDirty = true;

    UpdateCollisions(CamLoc);

    // ⭐ 从 ChunkMeshes 重建 LocalBounds（而不是只增不减），保证边界随卸载正确收缩。
    if (bBoundsDirty)
    {
        bBoundsDirty = false;

        FBox NewBounds(ForceInit);
        for (const auto& Pair : ChunkMeshes)
        {
            const FChunkKey& Key = Pair.Key;
            const float CS = VoxelConfig::ChunkSize * VoxelConfig::VoxelSize
                * (float)(1 << Key.LOD);
            const FVector Origin(Key.X * CS, Key.Y * CS, Key.Z * CS);
            NewBounds += FBox(Origin, Origin + FVector(CS, CS, CS));
        }
        LocalBounds = NewBounds.IsValid
            ? NewBounds
            : FBox(FVector(-100), FVector(100));

        UpdateBounds();
        MarkRenderTransformDirty();
    }
}

// ============================================================
// 生成回调
// ============================================================
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

    // ⭐ 只标记脏；AABB 在 StreamTick 尾部统一从 ChunkMeshes 重建。
    bBoundsDirty = true;

    // 空 chunk（全空气/全实心）不占 GPU 队列位置
    if (Mesh->Positions.Num() == 0 || Mesh->Indices.Num() == 0)
        return;

    FPendingAdd Item;
    Item.Key = Key;
    Item.Mesh = Mesh;
    PendingGPUAdds.Add(Item);
}

void UVoxelTerrainComponent::EndPlay(const EEndPlayReason::Type Reason)
{
    if (TickHandle.IsValid()) FTSTicker::GetCoreTicker().RemoveTicker(TickHandle);
    TickHandle.Reset();
    if (RenderTickHandle.IsValid()) FTSTicker::GetCoreTicker().RemoveTicker(RenderTickHandle);
    RenderTickHandle.Reset();
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

    // 加载期间也不完全停，只降速（避免玩家向下穿模）
    const bool bLoadInProgress =
        PendingGPUAdds.Num() > 0 || PendingGPURemoves.Num() > 0;
    const double MaxBudget = bLoadInProgress ? 0.003 : 0.005;

    // 移除优先（纯释放，很快）
    int32 RemoveBudget = 2;
    while (PendingCollisionRemoves.Num() > 0 && RemoveBudget > 0)
    {
        if (FPlatformTime::Seconds() - StartTime > MaxBudget) break;
        FChunkKey Key = PendingCollisionRemoves.Pop();
        RemoveCollisionForChunk(Key);
        --RemoveBudget;
    }

    // 取玩家位置（用于按距离优先创建碰撞）
    FVector CamLoc = GetComponentLocation();
    if (UWorld* World = GetWorld())
    {
        if (APlayerController* PC = World->GetFirstPlayerController())
        {
            if (APawn* Pawn = PC->GetPawnOrSpectator())
                CamLoc = Pawn->GetActorLocation();
        }
    }

    // 创建：加载期间 1 个，非加载期 2 个
    const int32 AddBudget = bLoadInProgress ? 1 : 2;
    int32 Added = 0;
    const float BaseChunkSize =
        VoxelConfig::ChunkSize * VoxelConfig::VoxelSize;

    while (Added < AddBudget && PendingCollisionAdds.Num() > 0)
    {
        if (FPlatformTime::Seconds() - StartTime > MaxBudget) break;

        // 遍历队列，找离玩家最近的 LOD0 chunk
        int32 BestIdx = -1;
        float BestDistSq = MAX_flt;
        for (int32 i = 0; i < PendingCollisionAdds.Num(); ++i)
        {
            const FChunkKey& K = PendingCollisionAdds[i];
            if (K.LOD != 0) continue;

            const FVector Center(
                (K.X + 0.5f) * BaseChunkSize,
                (K.Y + 0.5f) * BaseChunkSize,
                (K.Z + 0.5f) * BaseChunkSize);
            const float D = FVector::DistSquared(Center, CamLoc);
            if (D < BestDistSq)
            {
                BestDistSq = D;
                BestIdx = i;
            }
        }

        if (BestIdx < 0)
        {
            // 队列里没有 LOD0，清掉非 LOD0 的残留
            for (int32 i = PendingCollisionAdds.Num() - 1; i >= 0; --i)
                if (PendingCollisionAdds[i].LOD != 0)
                    PendingCollisionAdds.RemoveAt(i);
            break;
        }

        FChunkKey Key = PendingCollisionAdds[BestIdx];
        PendingCollisionAdds.RemoveAt(BestIdx);
        ++Added;

        TSharedPtr<FVoxelMeshData, ESPMode::ThreadSafe>* MeshPtr =
            ChunkMeshes.Find(Key);
        if (!MeshPtr || !MeshPtr->IsValid()) continue;

        CreateCollisionForChunk(Key, **MeshPtr);
    }
}