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

    // ⭐ 每帧处理碰撞队列
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
// 单个 chunk 生成（同步版本，目前未使用，保留兼容）
// ============================================================
void UVoxelTerrainComponent::GenerateOneChunk(const FChunkKey& Key)
{
    if (ChunkMeshes.Contains(Key)) return;

    // 1) 生成体素
    FVoxelGenerator Gen;
    FVoxelChunkData ChunkData;
    Gen.GenerateChunk(Key.X, Key.Y, Key.Z, Key.LOD, ChunkData.Voxels.GetData());

    // 2) 生成网格（局部坐标）
    auto MeshData = MakeShared<FVoxelMeshData, ESPMode::ThreadSafe>();
    FTransvoxelMesher::GenerateRegular(
        ChunkData, VoxelConfig::VoxelSize, Key.LOD, *MeshData);

    // 3) 把顶点偏移到世界坐标
    const FVector Offset = ChunkOriginWorld(Key);
    const FVector3f OffsetF(
        (float)Offset.X, (float)Offset.Y, (float)Offset.Z);
    for (FVector3f& P : MeshData->Positions)
        P += OffsetF;

    // 4) 存入 map
    ChunkMeshes.Add(Key, MeshData);
}

// ============================================================
// 主循环：加载 + 卸载
// ============================================================
#if 0
void UVoxelTerrainComponent::StreamTick()
{
    UWorld* World = GetWorld();
    if (!World) return;

    // 相机位置
    FVector CamLoc = GetComponentLocation();
    if (APlayerController* PC = World->GetFirstPlayerController())
    {
        if (APawn* Pawn = PC->GetPawnOrSpectator())
            CamLoc = Pawn->GetActorLocation();
    }

    // 使用 LOD 0 的 chunk 世界尺寸作为中心计算基准
    const float ChunkWorldSize = VoxelConfig::ChunkSize * VoxelConfig::VoxelSize;

    const int32 CenterX = FMath::FloorToInt(CamLoc.X / ChunkWorldSize);
    const int32 CenterY = FMath::FloorToInt(CamLoc.Y / ChunkWorldSize);
    const int32 CenterZ = FMath::FloorToInt(CamLoc.Z / ChunkWorldSize);

    const double Now = FPlatformTime::Seconds();

    // ============ 1) 加载 ============
    int32 NewRequests = 0;
    for (int32 dx = -ViewRadius; dx <= ViewRadius; ++dx)
        for (int32 dy = -ViewRadius; dy <= ViewRadius; ++dy)
            for (int32 dz = -ViewRadius; dz <= ViewRadius; ++dz)
            {
                // ⭐ 目前一律 LOD 0
                FChunkKey Key(CenterX + dx, CenterY + dy, CenterZ + dz, 0);

                // 已加载？刷新活跃时间
                if (ChunkMeshes.Contains(Key))
                {
                    ChunkLastActiveTime.Add(Key, Now);
                    continue;
                }

                // 已请求？跳过
                if (RequestedChunks.Contains(Key)) continue;
                RequestedChunks.Add(Key);

                // 异步生成
                TWeakObjectPtr<UVoxelTerrainComponent> WeakThis(this);
                Async(EAsyncExecution::ThreadPool, [WeakThis, Key]()
                    {
                        // 1) 生成体素（含 LOD）
                        FVoxelGenerator Gen;
                        FVoxelChunkData ChunkData;
                        Gen.GenerateChunk(
                            Key.X, Key.Y, Key.Z, Key.LOD, ChunkData.Voxels.GetData());

                        // 2) 生成网格（含 LOD）
                        auto MeshData = MakeShared<FVoxelMeshData, ESPMode::ThreadSafe>();
                        FTransvoxelMesher::GenerateRegular(
                            ChunkData, VoxelConfig::VoxelSize, Key.LOD, *MeshData);

                        // 3) 世界坐标偏移（含 LOD 缩放）
                        const float S = (float)(1 << Key.LOD);
                        const float LODChunkWorldSize =
                            VoxelConfig::ChunkSize * VoxelConfig::VoxelSize * S;
                        const FVector3f Offset(
                            Key.X * LODChunkWorldSize,
                            Key.Y * LODChunkWorldSize,
                            Key.Z * LODChunkWorldSize);
                        for (FVector3f& P : MeshData->Positions)
                            P += Offset;

                        // 4) 回主线程
                        AsyncTask(ENamedThreads::GameThread, [WeakThis, Key, MeshData]()
                            {
                                if (auto* Self = WeakThis.Get())
                                    Self->OnChunkGenerated(Key, MeshData);
                            });
                    });

                ++NewRequests;
            }

    if (NewRequests > 0)
    {
        UE_LOG(LogTemp, Log,
            TEXT("VoxelTerrain: requested %d new chunks (loaded=%d)"),
            NewRequests, ChunkMeshes.Num());
    }

    // ============ 2) 卸载 ============
    TArray<FChunkKey> ToUnload;
    for (auto& Pair : ChunkMeshes)
    {
        const FChunkKey& Key = Pair.Key;

        // Chebyshev 距离
        const int32 Dist = FMath::Max3(
            FMath::Abs(Key.X - CenterX),
            FMath::Abs(Key.Y - CenterY),
            FMath::Abs(Key.Z - CenterZ));

        if (Dist <= ViewRadius) continue;      // 仍在加载范围
        if (Dist <= UnloadRadius) continue;    // 仍在惰性带

        // 未到延迟时间？
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
        UE_LOG(LogTemp, Log,
            TEXT("VoxelTerrain: unloaded %d chunks (remaining=%d)"),
            ToUnload.Num(), ChunkMeshes.Num());
        bBoundsDirty = true;
    }

    // ============ 3) Bounds 更新 ============
    if (bBoundsDirty)
    {
        bBoundsDirty = false;
        UpdateBounds();
        MarkRenderTransformDirty();
    }
}
#else
void UVoxelTerrainComponent::StreamTick()
{
    UWorld* World = GetWorld();
    if (!World) return;


    // ---------- 相机位置 ----------
    FVector CamLoc = GetComponentLocation();
    if (APlayerController* PC = World->GetFirstPlayerController())
    {
        if (APawn* Pawn = PC->GetPawnOrSpectator())
        {
            CamLoc = Pawn->GetActorLocation();
        }
    }
    
    const double Now = FPlatformTime::Seconds();

    // ---------- 请求一个 chunk ----------
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

    // ---------- 基础网格坐标 ----------
    const FIntVector Base = WorldToBaseChunk(CamLoc);

    // LOD0 chunk 世界边长 3200
    const float LOD0ChunkSize =
        VoxelConfig::ChunkSize * VoxelConfig::VoxelSize;

    // LOD1 chunk 世界边长 6400
    const float LOD1ChunkSize =
        VoxelConfig::ChunkSize * VoxelConfig::VoxelSize * 2.f;

    // LOD0 覆盖的世界半径，用于跳过 LOD1 重叠
    const float LOD0WorldRadius =
        (LOD0Radius + 0.5f) * LOD0ChunkSize;

    // ---------- 1) LOD0 ----------
    for (int32 dx = -LOD0Radius; dx <= LOD0Radius; ++dx)
    {
        for (int32 dy = -LOD0Radius; dy <= LOD0Radius; ++dy)
        {
            for (int32 dz = -LOD0Radius; dz <= LOD0Radius; ++dz)
            {
                FChunkKey Key(Base.X + dx, Base.Y + dy, Base.Z + dz, 0);
                RequestChunk(Key);
            }
        }
    }

    // ---------- 2) LOD1 ----------
    // LOD1 坐标 = 基础坐标 / 2
    const int32 B1X = Base.X >> 1;
    const int32 B1Y = Base.Y >> 1;
    const int32 B1Z = Base.Z >> 1;
    const float LOD0WorldRadiusSqared = LOD0WorldRadius * LOD0WorldRadius;

    for (int32 dx = -LOD1Radius; dx <= LOD1Radius; ++dx)
    {
        for (int32 dy = -LOD1Radius; dy <= LOD1Radius; ++dy)
        {
            for (int32 dz = -LOD1Radius; dz <= LOD1Radius; ++dz)
            {
                const int32 X = B1X + dx;
                const int32 Y = B1Y + dy;
                const int32 Z = B1Z + dz;

                // 这个 LOD1 chunk 的世界中心
                const FVector ChunkCenter(
                    (X + 0.5f) * LOD1ChunkSize,
                    (Y + 0.5f) * LOD1ChunkSize,
                    (Z + 0.5f) * LOD1ChunkSize);

#if 0
                // 已经被 LOD0 覆盖，跳过
                if (FVector::Dist(ChunkCenter, CamLoc) < LOD0WorldRadius)
                    continue;
#else
                // 已经被 LOD0 覆盖，跳过
                if (FVector::DistSquared(ChunkCenter, CamLoc) < LOD0WorldRadiusSqared)
                    continue;
#endif

                FChunkKey Key(X, Y, Z, 1);
                RequestChunk(Key);
            }
        }
    }

    // ---------- 3) 卸载 ----------
    TArray<FChunkKey> ToUnload;
    for (auto& Pair : ChunkMeshes)
    {
        const FChunkKey& Key = Pair.Key;

        const float S = (float)(1 << Key.LOD);
        const float ChunkSize = LOD0ChunkSize * S;

        // chunk 世界中心
        const FVector ChunkCenter(
            (Key.X + 0.5f) * ChunkSize,
            (Key.Y + 0.5f) * ChunkSize,
            (Key.Z + 0.5f) * ChunkSize);

        const float Dist = FVector::Dist(ChunkCenter, CamLoc);

        float UnloadWorldRadius;
        if (Key.LOD == 0)
            UnloadWorldRadius = (LOD0Radius + 1.f) * LOD0ChunkSize;
        else
            UnloadWorldRadius = (LOD1Radius + 1.f) * LOD1ChunkSize;

        if (Dist < UnloadWorldRadius) continue;

        // 延迟卸载
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
    // ---------- 3.5) 碰撞 ----------
    UpdateCollisions(CamLoc);
    // ---------- 4) Bounds ----------
    if (bBoundsDirty)
    {
        bBoundsDirty = false;
        UpdateBounds();
        MarkRenderTransformDirty();
    }
}

#endif
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

    // 扩展 bounds
    for (const FVector3f& P : Mesh->Positions)
        LocalBounds += FVector(P);
    bBoundsDirty = true;

    // 推给 RT
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
    Super::EndPlay(Reason);
}

FIntVector UVoxelTerrainComponent::WorldToBaseChunk(const FVector& WorldPos) const
{
    const float BaseChunkSize =
        VoxelConfig::ChunkSize * VoxelConfig::VoxelSize;  // 3200
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
        // 先禁用碰撞，避免异步 cooking 残留的幽灵碰撞
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

        // 只给 LOD0 加碰撞
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
    // 删除预算大一些，优先级高
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

        // 队列等待期间 chunk 可能已被卸载
        TSharedPtr<FVoxelMeshData, ESPMode::ThreadSafe>* MeshPtr =
            ChunkMeshes.Find(Key);
        if (!MeshPtr || !MeshPtr->IsValid()) continue;

        CreateCollisionForChunk(Key, **MeshPtr);
    }
}