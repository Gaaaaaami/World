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

    CollisionProxy = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("VoxelCollisionProxy"));
    if (CollisionProxy)
    {
        CollisionProxy->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        CollisionProxy->SetCollisionObjectType(ECC_WorldStatic);
        CollisionProxy->SetCollisionResponseToAllChannels(ECR_Block);
        CollisionProxy->bUseComplexAsSimpleCollision = true;
        CollisionProxy->SetVisibility(false);
        CollisionProxy->SetHiddenInGame(true);
        CollisionProxy->SetGenerateOverlapEvents(false);
    }
}

void UVoxelTerrainComponent::BeginPlay()
{
    Super::BeginPlay();

    if (CollisionProxy && !CollisionProxy->GetAttachParent())
    {
        CollisionProxy->AttachToComponent(
            this,
            FAttachmentTransformRules::KeepRelativeTransform);
    }

    // 启动流式 ticker（每 0.2 秒跑一次）
    TickHandle = FTSTicker::GetCoreTicker().AddTicker(
        FTickerDelegate::CreateWeakLambda(this, [this](float)
            {
                StreamTick();
                return true;
            }), 0.2f);
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