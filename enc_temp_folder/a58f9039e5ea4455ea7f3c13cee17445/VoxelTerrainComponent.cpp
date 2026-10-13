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
void UVoxelTerrainComponent::GetUsedMaterials(TArray<UMaterialInterface*>& OutMaterials, bool bGetDebugMaterials) const
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
#if 0
void UVoxelTerrainComponent::BuildCollision()
{
    if (!CollisionProxy || !MeshDataPtr.IsValid()) return;

    const int32 NumVerts = MeshDataPtr->Positions.Num();
    const int32 NumTris = MeshDataPtr->Indices.Num() / 3;
    if (NumVerts == 0 || NumTris == 0) return;

    // 转成 PMC 用的类型
    TArray<FVector>       Positions;
    TArray<int32>         Indices;
    TArray<FVector>       Normals;
    TArray<FVector2D>     UVs;
    TArray<FLinearColor>  Colors;
    TArray<FProcMeshTangent> Tangents;

    Positions.SetNumUninitialized(NumVerts);
    Normals.SetNumUninitialized(NumVerts);
    UVs.SetNumUninitialized(NumVerts);
    Colors.SetNumUninitialized(NumVerts);
    Tangents.SetNumUninitialized(NumVerts);

    for (int32 i = 0; i < NumVerts; ++i)
    {
        Positions[i] = FVector(MeshDataPtr->Positions[i]);
        Normals[i] = MeshDataPtr->Normals.IsValidIndex(i)
            ? FVector(MeshDataPtr->Normals[i])
            : FVector(0, 0, 1);
        UVs[i] = FVector2D(0, 0);
        Colors[i] = FLinearColor::White;
        Tangents[i] = FProcMeshTangent(1, 0, 0);
    }

    Indices.SetNumUninitialized(MeshDataPtr->Indices.Num());
    for (int32 i = 0; i < MeshDataPtr->Indices.Num(); ++i)
        Indices[i] = (int32)MeshDataPtr->Indices[i];
    // bCreateCollision = true 是核心
    CollisionProxy->CreateMeshSection_LinearColor(
        0,
        Positions,
        Indices,
        Normals,
        UVs,
        Colors,
        Tangents,
        true /* bCreateCollision */);

    UE_LOG(LogTemp, Log,
        TEXT("VoxelTerrain: collision built, verts=%d tris=%d"),
        NumVerts, NumTris);
}
#endif

FVector UVoxelTerrainComponent::ChunkOriginWorld(const FChunkKey& Key) const
{
    const float ChunkWorldSize = VoxelConfig::ChunkSize * VoxelConfig::VoxelSize;
    return FVector(Key.X, Key.Y, Key.Z) * ChunkWorldSize;
}

void UVoxelTerrainComponent::GenerateOneChunk(const FChunkKey& Key)
{
    if (ChunkMeshes.Contains(Key)) return;

    // 1) 生成体素
    FVoxelGenerator Gen;
    FVoxelChunkData ChunkData;
    Gen.GenerateChunk(Key.X, Key.Y, Key.Z, ChunkData.Voxels.GetData());

    // 2) 生成网格（局部坐标，范围 [0, ChunkSize*VoxelSize]）
    auto MeshData = MakeShared<FVoxelMeshData, ESPMode::ThreadSafe>();
    FTransvoxelMesher::GenerateRegular(ChunkData, VoxelConfig::VoxelSize, *MeshData);

    // 3) 把顶点偏移到世界坐标
    const FVector Offset = ChunkOriginWorld(Key);
    const FVector3f OffsetF((float)Offset.X, (float)Offset.Y, (float)Offset.Z);
    for (FVector3f& P : MeshData->Positions)
        P += OffsetF;

    // 4) 存入 map
    ChunkMeshes.Add(Key, MeshData);
}

void UVoxelTerrainComponent::PushAllChunksToRenderThread()
{
    // 1) 合并所有 chunk 到一份 mesh data
    TSharedPtr<FVoxelMeshData, ESPMode::ThreadSafe> Merged =
        MakeShared<FVoxelMeshData, ESPMode::ThreadSafe>();
    Merged->Reset();

    FBox MergedBounds(ForceInit);

    for (auto& Pair : ChunkMeshes)
    {
        const FVoxelMeshData& M = *Pair.Value;
        const uint32 Base = (uint32)Merged->Positions.Num();

        Merged->Positions.Append(M.Positions);
        Merged->Normals.Append(M.Normals);
        Merged->Colors.Append(M.Colors);

        Merged->Indices.Reserve(Merged->Indices.Num() + M.Indices.Num());
        for (uint32 I : M.Indices)
            Merged->Indices.Add(I + Base);

        for (const FVector3f& P : M.Positions)
            MergedBounds += FVector(P);
    }

    // 2) 更新 bounds
    LocalBounds = MergedBounds;
    UpdateBounds();
    MarkRenderTransformDirty();

    // 3) 推给 render thread
    if (!SceneProxy) return;
    FVoxelSceneProxy* Proxy = static_cast<FVoxelSceneProxy*>(SceneProxy);

    UE_LOG(LogTemp, Log,
        TEXT("VoxelTerrain: merged, verts=%d tris=%d bounds=%s"),
        Merged->Positions.Num(),
        Merged->NumTriangles(),
        *LocalBounds.ToString());

    ENQUEUE_RENDER_COMMAND(UploadVoxelMesh)(
        [Proxy, Merged](FRHICommandListImmediate& RHICmdList)
        {
            if (Proxy)
                Proxy->UploadMeshData_RenderThread(RHICmdList, Merged);
        });
}

void UVoxelTerrainComponent::StreamTick()
{
    UWorld* World = GetWorld();
    if (!World) return;

    // 相机位置（先用组件自身位置做测试，后面换玩家）
    FVector CamLoc = GetComponentLocation();
    if (APlayerController* PC = World->GetFirstPlayerController())
    {
        if (APawn* Pawn = PC->GetPawnOrSpectator())
            CamLoc = Pawn->GetActorLocation();
    }

    // 相机所在的 chunk 坐标
    const float ChunkWorldSize = VoxelConfig::ChunkSize * VoxelConfig::VoxelSize;
    const int32 CenterX = FMath::FloorToInt(CamLoc.X / ChunkWorldSize);
    const int32 CenterY = FMath::FloorToInt(CamLoc.Y / ChunkWorldSize);
    const int32 CenterZ = FMath::FloorToInt(CamLoc.Z / ChunkWorldSize);

    // 遍历 R 半径内的所有 chunk
    int32 NewRequests = 0;
    for (int32 dx = -ViewRadius; dx <= ViewRadius; ++dx)
        for (int32 dy = -ViewRadius; dy <= ViewRadius; ++dy)
            for (int32 dz = -ViewRadius; dz <= ViewRadius; ++dz)
            {
                FChunkKey Key(CenterX + dx, CenterY + dy, CenterZ + dz);

                // 已请求过？跳过
                if (RequestedChunks.Contains(Key)) continue;
                RequestedChunks.Add(Key);

                // 异步生成这个 chunk
                TWeakObjectPtr<UVoxelTerrainComponent> WeakThis(this);
                Async(EAsyncExecution::ThreadPool, [WeakThis, Key]()
                    {
                        // 1) 体素数据（在后台线程生成）
                        FVoxelGenerator Gen;
                        FVoxelChunkData ChunkData;
                        Gen.GenerateChunk(Key.X, Key.Y, Key.Z, ChunkData.Voxels.GetData());

                        // 2) 网格（也在后台线程生成）
                        auto MeshData = MakeShared<FVoxelMeshData, ESPMode::ThreadSafe>();
                        FTransvoxelMesher::GenerateRegular(ChunkData, VoxelConfig::VoxelSize, *MeshData);

                        // 3) 世界坐标偏移
                        const float ChunkWorldSize = VoxelConfig::ChunkSize * VoxelConfig::VoxelSize;
                        const FVector3f Offset(
                            Key.X * ChunkWorldSize,
                            Key.Y * ChunkWorldSize,
                            Key.Z * ChunkWorldSize);
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
            TEXT("VoxelTerrain: requested %d new chunks (total pending/loaded: %d)"),
            NewRequests, RequestedChunks.Num());
    }
    if (bMeshDirty)
    {
        bMeshDirty = false;
        PushAllChunksToRenderThread();
    }
}
void UVoxelTerrainComponent::OnChunkGenerated(
    FChunkKey Key,
    TSharedPtr<FVoxelMeshData, ESPMode::ThreadSafe> Mesh)
{
    if (!Mesh.IsValid()) return;

    ChunkMeshes.Add(Key, Mesh);

    // 只标记，不立即 rebuild
    bMeshDirty = true;
}
void UVoxelTerrainComponent::EndPlay(const EEndPlayReason::Type Reason)
{
    if (TickHandle.IsValid())
        FTSTicker::GetCoreTicker().RemoveTicker(TickHandle);
    TickHandle.Reset();
    Super::EndPlay(Reason);
}