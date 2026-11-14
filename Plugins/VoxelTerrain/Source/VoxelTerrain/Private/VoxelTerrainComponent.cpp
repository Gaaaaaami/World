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
#include "Engine/Texture2D.h"
#include "ImageUtils.h"

#if 0
void DebugFBoxTest()
{
    // ... 原有代码 ...

    // ══════════════════════════════════════════════
    // FBox::operator+= 实验
    // ══════════════════════════════════════════════
    FBox A(FVector(0, 0, 0), FVector(100, 100, 100));
    FBox B(FVector(150, 50, 50), FVector(300, 80, 80));

    UE_LOG(LogTemp, Warning, TEXT("A: Min=(%.1f,%.1f,%.1f) Max=(%.1f,%.1f,%.1f)"),
        A.Min.X, A.Min.Y, A.Min.Z, A.Max.X, A.Max.Y, A.Max.Z);
    UE_LOG(LogTemp, Warning, TEXT("B: Min=(%.1f,%.1f,%.1f) Max=(%.1f,%.1f,%.1f)"),
        B.Min.X, B.Min.Y, B.Min.Z, B.Max.X, B.Max.Y, B.Max.Z);

    FBox C = A;
    C += B;

    UE_LOG(LogTemp, Warning, TEXT("C=A+=B: Min=(%.1f,%.1f,%.1f) Max=(%.1f,%.1f,%.1f)"),
        C.Min.X, C.Min.Y, C.Min.Z, C.Max.X, C.Max.Y, C.Max.Z);

    // 幂等性测试：加同一个 box 3 次
    FBox D = A;
    D += B;
    D += B;
    D += B;

    UE_LOG(LogTemp, Warning, TEXT("D=A+B+B+B: Min=(%.1f,%.1f,%.1f) Max=(%.1f,%.1f,%.1f)"),
        D.Min.X, D.Min.Y, D.Min.Z, D.Max.X, D.Max.Y, D.Max.Z);

    // 加一个完全被包含的 box
    FBox Inner(FVector(20, 20, 20), FVector(40, 40, 40));
    FBox E = A;
    E += Inner;

    UE_LOG(LogTemp, Warning, TEXT("E=A+Inner: Min=(%.1f,%.1f,%.1f) Max=(%.1f,%.1f,%.1f)"),
        E.Min.X, E.Min.Y, E.Min.Z, E.Max.X, E.Max.Y, E.Max.Z);
}
#endif

UVoxelTerrainComponent::UVoxelTerrainComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    bUseAsOccluder = false;
    CastShadow = true;
}

// ============================================================
// BeginPlay / EndPlay
// ============================================================
void UVoxelTerrainComponent::BeginPlay()
{
    Super::BeginPlay();


    if (TerrainMaterial)
    {
        TerrainMaterial->EnsureIsComplete();
    }
    InitBiomeTexture(); // 初始化生物群系的noise贴图
    LoadNoiseTextureData();
    PrewarmCollisionPool(256);  //初始化碰撞对象池
    PrewarmHISMPool(256);   //初始化植被对象池

    ChunkMeshes.Reserve(1024);
    ChunkVoxelData.Reserve(1024);
    ChunkLastActiveTime.Reserve(1024);
    //ChunkModifications.Reserve(1024);
    ChunkVersions.Reserve(1024);
    RequestedChunks.Reserve(1024);
    PendingPushToSceneProxy.Reserve(64);
    ChunkCollisionProxies.Reserve(64);

    // StreamTick：自然加载/卸载（异步）
    TickHandle = FTSTicker::GetCoreTicker().AddTicker(
        FTickerDelegate::CreateWeakLambda(this, [this](float)
            {
                StreamTick();
                return true;
            }), 0.16f);

    // CollisionTick：自然场景的碰撞回收 + 创建
    // 编辑触发的碰撞更新已在 RebuildTouchedChunks 里同步处理，不走这里
    CollisionTickHandle = FTSTicker::GetCoreTicker().AddTicker(
        FTickerDelegate::CreateWeakLambda(this, [this](float)
            {
                ProcessCollisionQueue();
                return true;
            }), 0.33f);

    EditTickHandle = FTSTicker::GetCoreTicker().AddTicker(
        FTickerDelegate::CreateWeakLambda(this, [this](float)
            {
                EditTaskTick();
                return true;
            }), 0.22f);

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

    if (EditTickHandle.IsValid())
        FTSTicker::GetCoreTicker().RemoveTicker(EditTickHandle);
    EditTickHandle.Reset();

    Super::EndPlay(Reason);
}

// ============================================================
// 噪声贴图加载
// ============================================================
void UVoxelTerrainComponent::LoadNoiseTextureData()
{
#if VOXEL_USE_NOISE_TEXTURE
    TSharedPtr<TArray<FVoxelGenerator::FNoiseLayerData>> NewLayers2D = MakeShared<TArray<FVoxelGenerator::FNoiseLayerData>>();
    NewLayers2D->Reserve(NoiseLayers.Num());

    for (const FVoxelNoiseLayerInfo& LayerInfo : NoiseLayers)
    {
        if (!LayerInfo.Texture) continue;

        FImage SourceImage;
        if (LayerInfo.Texture->Source.GetMipImage(SourceImage, 0))
        {
            const int32 SX = SourceImage.SizeX;
            const int32 SY = SourceImage.SizeY;
            if (SX != SY || SX < 4) continue;

            TArray<float> F;
            F.SetNumUninitialized(SX * SY);
            const int32 NumPixels = SX * SY;
            const uint8* P = (const uint8*)SourceImage.RawData.GetData();

            if (SourceImage.Format == ERawImageFormat::G8)
            {
                for (int32 i = 0; i < NumPixels; ++i) F[i] = P[i] / 255.f;
            }
            else if (SourceImage.Format == ERawImageFormat::BGRA8)
            {
                for (int32 i = 0; i < NumPixels; ++i) F[i] = P[i * 4 + 2] / 255.f;
            }
            else
            {
                FImage RGBAImage;
                SourceImage.CopyTo(RGBAImage, ERawImageFormat::BGRA8, EGammaSpace::Linear);
                const uint8* RGBA_P = (const uint8*)RGBAImage.RawData.GetData();
                for (int32 i = 0; i < NumPixels; ++i) F[i] = RGBA_P[i * 4 + 2] / 255.f;
            }

            FVoxelGenerator::FNoiseLayerData NewLayer;
            NewLayer.Data = MakeShared<TArray<float>>(MoveTemp(F));
            NewLayer.Size = SX;
            NewLayer.Frequency = LayerInfo.Frequency;
            NewLayer.Amplitude = LayerInfo.Amplitude;
            NewLayer.Offset = LayerInfo.Offset;
            NewLayers2D->Add(NewLayer);
        }
    }

    FVoxelGenerator::GlobalNoiseLayers = NewLayers2D;

    TSharedPtr<TArray<FVoxelGenerator::FNoiseLayer3DData>> NewLayers3D = MakeShared<TArray<FVoxelGenerator::FNoiseLayer3DData>>();
    NewLayers3D->Reserve(NoiseLayers3D.Num());

    for (const FVoxelNoiseLayer3DInfo& LayerInfo : NoiseLayers3D)
    {
        if (!LayerInfo.Texture) continue;

        FImage SourceImage;
        if (LayerInfo.Texture->Source.GetMipImage(SourceImage, 0))
        {
            const int32 SX = SourceImage.SizeX;
            const int32 SY = SourceImage.SizeY;
            const int32 SZ = LayerInfo.Texture->Source.GetVolumeSizeZ();
            if (SX != SY || SX != SZ || SX < 4) continue;

            TArray<float> F;
            F.SetNumUninitialized(SX * SY * SZ);
            const int32 NumPixels = SX * SY * SZ;
            const uint8* P = (const uint8*)SourceImage.RawData.GetData();

            if (SourceImage.Format == ERawImageFormat::G8)
            {
                for (int32 i = 0; i < NumPixels; ++i) F[i] = P[i] / 255.f;
            }
            else if (SourceImage.Format == ERawImageFormat::BGRA8)
            {
                for (int32 i = 0; i < NumPixels; ++i) F[i] = P[i * 4 + 2] / 255.f;
            }
            else
            {
                FImage RGBAImage;
                SourceImage.CopyTo(RGBAImage, ERawImageFormat::BGRA8, EGammaSpace::Linear);
                const uint8* RGBA_P = (const uint8*)RGBAImage.RawData.GetData();
                for (int32 i = 0; i < NumPixels; ++i) F[i] = RGBA_P[i * 4 + 2] / 255.f;
            }

            FVoxelGenerator::FNoiseLayer3DData NewLayer;
            NewLayer.Data = MakeShared<TArray<float>>(MoveTemp(F));
            NewLayer.Size = SX;
            NewLayer.Frequency = LayerInfo.Frequency;
            NewLayer.Amplitude = LayerInfo.Amplitude;
            NewLayer.Offset = LayerInfo.Offset;
            NewLayers3D->Add(NewLayer);
        }
    }

    FVoxelGenerator::GlobalNoiseLayers3D = NewLayers3D;
#endif
}

// ============================================================
// Proxy / Bounds / Materials
// ============================================================
FPrimitiveSceneProxy* UVoxelTerrainComponent::CreateSceneProxy()
{
    return new FVoxelSceneProxy(this, TerrainMaterial);
}

FBoxSphereBounds UVoxelTerrainComponent::CalcBounds(const FTransform& LocalToWorld) const
{
    if (LocalBounds.IsValid)
        return FBoxSphereBounds(LocalBounds);
    return FBoxSphereBounds(FBox(FVector(-100000.f), FVector(100000.f)));
}

void UVoxelTerrainComponent::GetUsedMaterials(
    TArray<UMaterialInterface*>& OutMaterials, bool bGetDebugMaterials) const
{
    if (TerrainMaterial)
        OutMaterials.Add(TerrainMaterial);
    else if (UMaterial* DefaultMat = UMaterial::GetDefaultMaterial(MD_Surface))
        OutMaterials.Add(DefaultMat);
}

// ============================================================
// 坐标转换
// ============================================================
FIntVector UVoxelTerrainComponent::WorldToBaseChunk(const FVector& WorldPos) const
{
    const float BaseChunkSize = VoxelConfig::ChunkSize * VoxelConfig::VoxelSize;
    return FIntVector(
        FMath::FloorToInt(WorldPos.X / BaseChunkSize),
        FMath::FloorToInt(WorldPos.Y / BaseChunkSize),
        FMath::FloorToInt(WorldPos.Z / BaseChunkSize));
}

// ============================================================
// Chunk 异步生成（自然加载用）
//    ⭐ 主线程收集 Mods 快照，后台不碰主线程数据
//    ⭐ 把 ChunkData 一起传回主线程，供 ApplyEdit 缓存
// ============================================================
void UVoxelTerrainComponent::SpawnChunkAsyncTask(const FChunkKey& Key, int32 CurrentVersion)
{
    // 主线程：收集 27 个邻居 chunk 的修改数据快照
    //FVoxelModificationMap ModsSnapshot;
    //if (ChunkModifications.Num() > 0)
    //{
    //    int32 MinX, MaxX, MinY, MaxY, MinZ, MaxZ;
    //    if (Key.LOD == 0) {
    //        MinX = Key.X - 1; MaxX = Key.X + 1;
    //        MinY = Key.Y - 1; MaxY = Key.Y + 1;
    //        MinZ = Key.Z - 1; MaxZ = Key.Z + 1;
    //    }
    //    else {
    //        int32 S = 1 << Key.LOD;
    //        MinX = (Key.X * S) - 1; MaxX = ((Key.X + 1) * S);
    //        MinY = (Key.Y * S) - 1; MaxY = ((Key.Y + 1) * S);
    //        MinZ = (Key.Z * S) - 1; MaxZ = ((Key.Z + 1) * S);
    //    }

    //    for (int32 x = MinX; x <= MaxX; ++x)
    //        for (int32 y = MinY; y <= MaxY; ++y)
    //            for (int32 z = MinZ; z <= MaxZ; ++z)
    //                if (const FVoxelModificationMap* M = ChunkModifications.Find(FChunkKey(x, y, z, 0)))
    //                    ModsSnapshot.Append(*M);
    //}

    TWeakObjectPtr<UVoxelTerrainComponent> WeakThis(this);

    Async(EAsyncExecution::ThreadPool,
        [WeakThis, Key, CurrentVersion /*, ModsSnapshot = MoveTemp(ModsSnapshot)*/]()
        {
            FVoxelGenerator Gen;
            FVoxelChunkData ChunkData;
            Gen.GenerateChunk(Key.X, Key.Y, Key.Z, Key.LOD, /*ChunkData.Voxels.GetData()*/ ChunkData);

            auto MeshData = MakeShared<FVoxelMeshData, ESPMode::ThreadSafe>();

            const float S = (float)(1 << Key.LOD);
            const float LODChunkWorldSize = VoxelConfig::ChunkSize * VoxelConfig::VoxelSize * S;
            const FVector3f Offset(
                Key.X * LODChunkWorldSize,
                Key.Y * LODChunkWorldSize,
                Key.Z * LODChunkWorldSize);

            FTransvoxelMesher::GenerateRegular(
                ChunkData, VoxelConfig::VoxelSize, Key.LOD, Offset, *MeshData);

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

            // ⭐ 把 ChunkData 一起传回主线程（供 ApplyEdit 读原始密度）
            auto VoxelDataShared = MakeShared<FVoxelChunkData>(MoveTemp(ChunkData));

            AsyncTask(ENamedThreads::GameThread,
                [WeakThis, Key, MeshData, VoxelDataShared, CurrentVersion]()
                {
                    if (auto* Self = WeakThis.Get())
                        Self->OnChunkGenerated(Key, MeshData, VoxelDataShared, CurrentVersion);
                });
        });
}

// ============================================================
// StreamTick（自然加载/卸载，异步，走 tick 没问题）
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

    // ============ 加载候选收集 ============
    struct FLoadCandidate
    {
        float     DistSq = 0.f;
        FChunkKey Key;
    };
    TArray<FLoadCandidate> Candidates;
    Candidates.Reserve(1024);

    const float BaseChunkSize = VoxelConfig::ChunkSize * VoxelConfig::VoxelSize;
    const FIntVector Base = WorldToBaseChunk(CamLoc);

    const int32 RadiusChunkXY = LOD0RadiusChunksXY;
    const int32 RadiusChunkZ = LOD0RadiusChunksZ;

    for (int32 dx = -RadiusChunkXY; dx <= RadiusChunkXY; ++dx)
    {
        for (int32 dy = -RadiusChunkXY; dy <= RadiusChunkXY; ++dy)
        {
            for (int32 dz = -RadiusChunkZ; dz <= RadiusChunkZ; ++dz)
            {
                FChunkKey Key(Base.X + dx, Base.Y + dy, Base.Z + dz, 0);

                if (ChunkMeshes.Contains(Key))
                {
                    ChunkLastActiveTime.FindOrAdd(Key) = Now;
                    continue;
                }

                if (RequestedChunks.Contains(Key)) continue;

                const FVector Center(
                    (Key.X + 0.5f) * BaseChunkSize,
                    (Key.Y + 0.5f) * BaseChunkSize,
                    (Key.Z + 0.5f) * BaseChunkSize);

                Candidates.Add({ (float)FVector::DistSquared(Center, CamLoc), Key });
            }
        }
    }

    const float SpeedSq = Pawn->GetVelocity().SizeSquared();
    int32 MaxRequestsPerTick = 8;
    if (SpeedSq > 4000000.f)   MaxRequestsPerTick = 16;
    if (SpeedSq > 10000000.f)  MaxRequestsPerTick = 24;

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

    // ============ 卸载 ============
    const float UnloadRadiusXY = (LOD0RadiusChunksXY + 1) * BaseChunkSize;
    const float UnloadRadiusZ = (LOD0RadiusChunksZ + 1) * BaseChunkSize;

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
        ChunkVoxelData.Remove(Key);   // ⭐ 清理体素缓存
        ChunkLastActiveTime.Remove(Key);
        RequestedChunks.Remove(Key);
        PendingPushToSceneProxy.Remove(Key);
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

    if (ToUnload.Num() > 0)
    {
        LocalBounds = FBox(ForceInit);
        for (const auto& Pair : ChunkMeshes)
        {
            if (Pair.Value.IsValid() && Pair.Value->Bounds.IsValid)
                LocalBounds += Pair.Value->Bounds;
        }
        bBoundsDirty = true;
    }

    if (bBoundsDirty)
    {
        bBoundsDirty = false;
        UpdateBounds();
        MarkRenderTransformDirty();
    }
}

// ============================================================
// OnChunkGenerated（自然加载的回调，编辑不走这里）
// ============================================================
void UVoxelTerrainComponent::OnChunkGenerated(
    FChunkKey Key,
    TSharedPtr<FVoxelMeshData, ESPMode::ThreadSafe> Mesh,
    TSharedPtr<FVoxelChunkData> VoxelData,
    int32 Version)
{
    RequestedChunks.Remove(Key);

    if (!Mesh.IsValid()) return;

    // 版本不匹配 → 丢弃（编辑期间版本号会变，让编辑的同步结果胜出）
    if (Version != ChunkVersions.FindRef(Key)) return;

    ChunkMeshes.Add(Key, Mesh);

    // ⭐ 缓存体素数据（供 ApplyEdit 读原始密度）
    if (VoxelData.IsValid())
    {
        ChunkVoxelData.Add(Key, VoxelData);
    }

    ChunkLastActiveTime.Add(Key, FPlatformTime::Seconds());

    if (Mesh->Bounds.IsValid)
    {
        if (LocalBounds.IsValid)
            LocalBounds += Mesh->Bounds;
        else
            LocalBounds = Mesh->Bounds;
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

    DispatchVegetationTask(Key, Mesh);  // 渲染植物线程

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
// ApplyEdit
//   ⭐ 优化点：
//     1. NeighborData[27] 缓存 + LastChunkCoord 去重（省 32000 次 TMap::Find）
//     2. 主 chunk 未加载 → continue（删掉 SampleDensity 兜底）
//     3. halo 边界修正（LocalX <= 1 和 LocalX >= ChunkN-1 都算边缘）
//     4. MainMods 按 ChunkCoord 缓存
// ============================================================

void UVoxelTerrainComponent::ApplyEdit(const FVector& WorldPos, float Radius, bool bIsAdd)
{
    constexpr int32 ChunkN = VoxelConfig::ChunkSize;
    constexpr int32 MaxHalo = ChunkN + 1;

    const float VoxelSize = VoxelConfig::VoxelSize;
    const float Scale = VoxelSize * 1.5f;
    const float InvScale = 1.0f / Scale;
    const float SearchRadius = Radius + Scale;
    const float SearchRadiusSq = SearchRadius * SearchRadius;

    const float WorldX = WorldPos.X, WorldY = WorldPos.Y, WorldZ = WorldPos.Z;
    const FVector GridPos = WorldPos / VoxelSize;
    const int32 RadiusInVoxels = FMath::CeilToInt(SearchRadius / VoxelSize);

    const FIntVector MinGrid(
        FMath::FloorToInt(GridPos.X - RadiusInVoxels - 1),
        FMath::FloorToInt(GridPos.Y - RadiusInVoxels - 1),
        FMath::FloorToInt(GridPos.Z - RadiusInVoxels - 1));
    const FIntVector MaxGrid(
        FMath::FloorToInt(GridPos.X + RadiusInVoxels + 1),
        FMath::FloorToInt(GridPos.Y + RadiusInVoxels + 1),
        FMath::FloorToInt(GridPos.Z + RadiusInVoxels + 1));

    TSet<FChunkKey> TouchedChunks;
    TouchedChunks.Reserve(16);

    const auto FloorDiv = [](int32 V, int32 D) -> int32
        {
            const int32 Q = V / D, R = V % D;
            return Q - ((R != 0 && R < 0) ? 1 : 0);
        };

    // ═════════════ Phase 1：按 Chunk 遍历编辑 ═════════════
    const int32 MinCX = FloorDiv(MinGrid.X, ChunkN);
    const int32 MaxCX = FloorDiv(MaxGrid.X, ChunkN);
    const int32 MinCY = FloorDiv(MinGrid.Y, ChunkN);
    const int32 MaxCY = FloorDiv(MaxGrid.Y, ChunkN);
    const int32 MinCZ = FloorDiv(MinGrid.Z, ChunkN);
    const int32 MaxCZ = FloorDiv(MaxGrid.Z, ChunkN);

    for (int32 cx = MinCX; cx <= MaxCX; ++cx)
    {
        for (int32 cy = MinCY; cy <= MaxCY; ++cy)
        {
            for (int32 cz = MinCZ; cz <= MaxCZ; ++cz)
            {
                const FChunkKey MainKey(cx, cy, cz, 0);
                FVoxelChunkData* MainChunk = nullptr;
                if (auto* P = ChunkVoxelData.Find(MainKey))
                {
                    if (P->IsValid()) MainChunk = P->Get();
                }
                if (!MainChunk) continue;

                FVoxelChunkData* NeighborData[27] = {};
                int32 Idx = 0;
                for (int32 cdx = -1; cdx <= 1; ++cdx)
                    for (int32 cdy = -1; cdy <= 1; ++cdy)
                        for (int32 cdz = -1; cdz <= 1; ++cdz)
                        {
                            const FChunkKey NK(cx + cdx, cy + cdy, cz + cdz, 0);
                            if (auto* N = ChunkVoxelData.Find(NK))
                                if (N->IsValid()) NeighborData[Idx] = N->Get();
                            ++Idx;
                        }

                const int32 KVX = cx * ChunkN;
                const int32 KVY = cy * ChunkN;
                const int32 KVZ = cz * ChunkN;

                const int32 VMinX = (MinGrid.X > KVX) ? MinGrid.X : KVX;
                const int32 VMaxX = (MaxGrid.X < KVX + ChunkN - 1) ? MaxGrid.X : (KVX + ChunkN - 1);
                const int32 VMinY = (MinGrid.Y > KVY) ? MinGrid.Y : KVY;
                const int32 VMaxY = (MaxGrid.Y < KVY + ChunkN - 1) ? MaxGrid.Y : (KVY + ChunkN - 1);
                const int32 VMinZ = (MinGrid.Z > KVZ) ? MinGrid.Z : KVZ;
                const int32 VMaxZ = (MaxGrid.Z < KVZ + ChunkN - 1) ? MaxGrid.Z : (KVZ + ChunkN - 1);

                bool bChunkDirty = false;
                bool bNeedNegX = false, bNeedPosX = false;
                bool bNeedNegY = false, bNeedPosY = false;
                bool bNeedNegZ = false, bNeedPosZ = false;

                // ⭐ 浮点乘法改为增量步进，大幅减少指令数
                float DX = VMinX * VoxelSize - WorldX;
                for (int32 x = VMinX; x <= VMaxX; ++x, DX += VoxelSize)
                {
                    const float DX2 = DX * DX;

                    float DY = VMinY * VoxelSize - WorldY;
                    for (int32 y = VMinY; y <= VMaxY; ++y, DY += VoxelSize)
                    {
                        const float DXY2 = DX2 + DY * DY;
                        if (DXY2 > SearchRadiusSq) continue;

                        float DZ = VMinZ * VoxelSize - WorldZ;
                        for (int32 z = VMinZ; z <= VMaxZ; ++z, DZ += VoxelSize)
                        {
                            const float DistSq = DXY2 + DZ * DZ;
                            if (DistSq > SearchRadiusSq) continue;

                            // ⭐ 干掉 FMath 函数调用
                            const float Dist = sqrtf(DistSq);
                            const float SDF = bIsAdd ? (Dist - Radius) : (Radius - Dist);
                            float W = SDF * InvScale;
                            if (W < -1.0f) W = -1.0f; else if (W > 1.0f) W = 1.0f;
                            const int8 SDFWeight = (int8)(W * 127.0f + (W >= 0.0f ? 0.5f : -0.5f));

                            const int32 LX0 = x - KVX;
                            const int32 LY0 = y - KVY;
                            const int32 LZ0 = z - KVZ;

                            const int8 CurrentWeight = MainChunk->At(LX0, LY0, LZ0).Weight;

                            // ⭐ 干掉 FMath::Min/Max<int8>
                            const int8 FinalWeight = bIsAdd
                                ? (CurrentWeight < SDFWeight ? CurrentWeight : SDFWeight)
                                : (CurrentWeight > SDFWeight ? CurrentWeight : SDFWeight);

                            bChunkDirty = true;

                            const bool bNegX = (LX0 <= 1);
                            const bool bPosX = (LX0 >= ChunkN - 2);
                            const bool bNegY = (LY0 <= 1);
                            const bool bPosY = (LY0 >= ChunkN - 2);
                            const bool bNegZ = (LZ0 <= 1);
                            const bool bPosZ = (LZ0 >= ChunkN - 2);

                            if (bNegX) bNeedNegX = true;
                            if (bPosX) bNeedPosX = true;
                            if (bNegY) bNeedNegY = true;
                            if (bPosY) bNeedPosY = true;
                            if (bNegZ) bNeedNegZ = true;
                            if (bPosZ) bNeedPosZ = true;

                            int32 XOffsets[2] = { 0, 0 };
                            int32 YOffsets[2] = { 0, 0 };
                            int32 ZOffsets[2] = { 0, 0 };
                            int32 NX = 1, NY = 1, NZ = 1;
                            if (bNegX) XOffsets[NX++] = -1; else if (bPosX) XOffsets[NX++] = 1;
                            if (bNegY) YOffsets[NY++] = -1; else if (bPosY) YOffsets[NY++] = 1;
                            if (bNegZ) ZOffsets[NZ++] = -1; else if (bPosZ) ZOffsets[NZ++] = 1;

                            for (int32 ix = 0; ix < NX; ++ix)
                            {
                                const int32 DXOff = XOffsets[ix];
                                const int32 LX = LX0 - DXOff * ChunkN;

                                for (int32 iy = 0; iy < NY; ++iy)
                                {
                                    const int32 DYOff = YOffsets[iy];
                                    const int32 LY = LY0 - DYOff * ChunkN;

                                    for (int32 iz = 0; iz < NZ; ++iz)
                                    {
                                        const int32 DZOff = ZOffsets[iz];
                                        const int32 LZ = LZ0 - DZOff * ChunkN;

                                        const int32 NIdx = (DXOff + 1) * 9 + (DYOff + 1) * 3 + (DZOff + 1);
                                        FVoxelChunkData* Data = NeighborData[NIdx];
                                        if (!Data) continue;
                                        if (LX < -1 || LX > MaxHalo ||
                                            LY < -1 || LY > MaxHalo ||
                                            LZ < -1 || LZ > MaxHalo) continue;

                                        Data->At(LX, LY, LZ).Weight = FinalWeight;
                                    }
                                }
                            }
                        }
                    }
                }

                if (bChunkDirty)
                {
                    TouchedChunks.Add(MainKey);
                    if (bNeedNegX) TouchedChunks.Add(FChunkKey(cx - 1, cy, cz, 0));
                    if (bNeedPosX) TouchedChunks.Add(FChunkKey(cx + 1, cy, cz, 0));
                    if (bNeedNegY) TouchedChunks.Add(FChunkKey(cx, cy - 1, cz, 0));
                    if (bNeedPosY) TouchedChunks.Add(FChunkKey(cx, cy + 1, cz, 0));
                    if (bNeedNegZ) TouchedChunks.Add(FChunkKey(cx, cy, cz - 1, 0));
                    if (bNeedPosZ) TouchedChunks.Add(FChunkKey(cx, cy, cz + 1, 0));

                    if (bNeedNegX && bNeedNegY) TouchedChunks.Add(FChunkKey(cx - 1, cy - 1, cz, 0));
                    if (bNeedNegX && bNeedPosY) TouchedChunks.Add(FChunkKey(cx - 1, cy + 1, cz, 0));
                    if (bNeedPosX && bNeedNegY) TouchedChunks.Add(FChunkKey(cx + 1, cy - 1, cz, 0));
                    if (bNeedPosX && bNeedPosY) TouchedChunks.Add(FChunkKey(cx + 1, cy + 1, cz, 0));

                    if (bNeedNegX && bNeedNegZ) TouchedChunks.Add(FChunkKey(cx - 1, cy, cz - 1, 0));
                    if (bNeedNegX && bNeedPosZ) TouchedChunks.Add(FChunkKey(cx - 1, cy, cz + 1, 0));
                    if (bNeedPosX && bNeedNegZ) TouchedChunks.Add(FChunkKey(cx + 1, cy, cz - 1, 0));
                    if (bNeedPosX && bNeedPosZ) TouchedChunks.Add(FChunkKey(cx + 1, cy, cz + 1, 0));

                    if (bNeedNegY && bNeedNegZ) TouchedChunks.Add(FChunkKey(cx, cy - 1, cz - 1, 0));
                    if (bNeedNegY && bNeedPosZ) TouchedChunks.Add(FChunkKey(cx, cy - 1, cz + 1, 0));
                    if (bNeedPosY && bNeedNegZ) TouchedChunks.Add(FChunkKey(cx, cy + 1, cz - 1, 0));
                    if (bNeedPosY && bNeedPosZ) TouchedChunks.Add(FChunkKey(cx, cy + 1, cz + 1, 0));
                }
            }
        }
    }

    if (TouchedChunks.IsEmpty()) return;

    // ═════════════ Phase 2：每个 chunk 局部重建 ═════════════
    const double Now = FPlatformTime::Seconds();
    constexpr int32 CellN = VoxelConfig::ChunkSize;

    for (const FChunkKey& Key : TouchedChunks)
    {
        auto* CachedPtr = ChunkVoxelData.Find(Key);
        if (!CachedPtr || !CachedPtr->IsValid()) continue;

        auto* MeshPtr = ChunkMeshes.Find(Key);
        if (!MeshPtr || !MeshPtr->IsValid()) continue;

        const int32 KVX = Key.X * CellN;
        const int32 KVY = Key.Y * CellN;
        const int32 KVZ = Key.Z * CellN;

        const int32 OMinX = (KVX - 2 > MinGrid.X) ? (KVX - 2) : MinGrid.X;
        const int32 OMinY = (KVY - 2 > MinGrid.Y) ? (KVY - 2) : MinGrid.Y;
        const int32 OMinZ = (KVZ - 2 > MinGrid.Z) ? (KVZ - 2) : MinGrid.Z;
        const int32 OMaxX = (KVX + CellN + 1 < MaxGrid.X) ? (KVX + CellN + 1) : MaxGrid.X;
        const int32 OMaxY = (KVY + CellN + 1 < MaxGrid.Y) ? (KVY + CellN + 1) : MaxGrid.Y;
        const int32 OMaxZ = (KVZ + CellN + 1 < MaxGrid.Z) ? (KVZ + CellN + 1) : MaxGrid.Z;
        if (OMinX > OMaxX || OMinY > OMaxY || OMinZ > OMaxZ) continue;

        const FIntVector CellMin(
            (OMinX - KVX - 2 > 0) ? (OMinX - KVX - 2) : 0,
            (OMinY - KVY - 2 > 0) ? (OMinY - KVY - 2) : 0,
            (OMinZ - KVZ - 2 > 0) ? (OMinZ - KVZ - 2) : 0);
        const FIntVector CellMax(
            (OMaxX - KVX + 1 < CellN - 1) ? (OMaxX - KVX + 1) : (CellN - 1),
            (OMaxY - KVY + 1 < CellN - 1) ? (OMaxY - KVY + 1) : (CellN - 1),
            (OMaxZ - KVZ + 1 < CellN - 1) ? (OMaxZ - KVZ + 1) : (CellN - 1));
        if (CellMin.X > CellMax.X || CellMin.Y > CellMax.Y || CellMin.Z > CellMax.Z) continue;

        ChunkVersions.FindOrAdd(Key)++;

        FVoxelMeshData& MeshData = **MeshPtr;
        const FVector3f Offset(KVX * VoxelSize, KVY * VoxelSize, KVZ * VoxelSize);

        FTransvoxelMesher::GenerateRegularPartial(
            **CachedPtr, VoxelSize, Offset, CellMin, CellMax, MeshData);

        // ---- Bounds 重算（手写极速版，绕过 FBox 开销）----
        FBox MeshBounds(ForceInit);
        const int32 NumVerts = MeshData.Positions.Num();
        if (NumVerts > 0)
        {
            const FVector3f* PData = MeshData.Positions.GetData();
            FVector MinV(FLT_MAX, FLT_MAX, FLT_MAX);
            FVector MaxV(-FLT_MAX, -FLT_MAX, -FLT_MAX);
            for (int32 i = 0; i < NumVerts; ++i)
            {
                const FVector3f& P = PData[i];
                if (P.X < MinV.X) MinV.X = P.X; else if (P.X > MaxV.X) MaxV.X = P.X;
                if (P.Y < MinV.Y) MinV.Y = P.Y; else if (P.Y > MaxV.Y) MaxV.Y = P.Y;
                if (P.Z < MinV.Z) MinV.Z = P.Z; else if (P.Z > MaxV.Z) MaxV.Z = P.Z;
            }
            MeshBounds = FBox(MinV, MaxV);
        }
        MeshData.Bounds = MeshBounds;

        ChunkLastActiveTime[Key] = Now;
        if (MeshData.Bounds.IsValid)
        {
            if (LocalBounds.IsValid) LocalBounds += MeshData.Bounds;
            else                     LocalBounds = MeshData.Bounds;
        }
        bBoundsDirty = true;

        // ---- 推渲染：深拷贝 ----
        if (FVoxelSceneProxy* Proxy = static_cast<FVoxelSceneProxy*>(SceneProxy))
        {
            auto MeshForRender = MakeShared<FVoxelMeshData, ESPMode::ThreadSafe>(MeshData);
            ENQUEUE_RENDER_COMMAND(AddVoxelChunk)(
                [Proxy, Key, MeshForRender](FRHICommandListImmediate& RHICmdList)
                {
                    Proxy->AddChunk_RenderThread(RHICmdList, Key, MeshForRender);
                });
        }
        else
        {
            PendingPushToSceneProxy.Add(Key);
        }

        // ---- PMC 碰撞 ----
        if (UProceduralMeshComponent* PMC = ChunkCollisionProxies.FindRef(Key).Get())
        {
            if (IsValid(PMC))
            {
                const int32 NumIndices = MeshData.Indices.Num();

                TArray<FVector> V;
                TArray<int32> T;
                V.SetNumUninitialized(NumVerts);
                T.SetNumUninitialized(NumIndices);

                const FVector3f* PData = MeshData.Positions.GetData();
                FVector* VData = V.GetData();
                for (int32 i = 0; i < NumVerts; ++i)
                {
                    VData[i] = FVector(PData[i]);
                }

                if (NumIndices > 0)
                {
                    FMemory::Memcpy(T.GetData(), MeshData.Indices.GetData(), NumIndices * sizeof(int32));
                }

                PMC->CreateMeshSection_LinearColor(
                    0, V, T,
                    TArray<FVector>(), TArray<FVector2D>(),
                    TArray<FLinearColor>(), TArray<FProcMeshTangent>(),
                    true, true);
            }
        }
    }
}


void UVoxelTerrainComponent::PrewarmCollisionPool(int32 PoolSize)
{
    if (PoolSize <= 0) return;

    CollisionComponentPool.Reserve(PoolSize);

    for (int32 i = 0; i < PoolSize; ++i)
    {
        UProceduralMeshComponent* PMC = NewObject<UProceduralMeshComponent>(this, NAME_None, RF_Transient);
        if (!PMC) continue;

        PMC->SetupAttachment(this);
        PMC->bUseAsyncCooking = true;
        PMC->bUseComplexAsSimpleCollision = true;
        PMC->SetCollisionObjectType(ECC_WorldStatic);
        PMC->SetCollisionResponseToAllChannels(ECR_Block);
        PMC->SetGenerateOverlapEvents(false);
        PMC->SetHiddenInGame(true);
        PMC->SetVisibility(false);
        PMC->RegisterComponent();
        PMC->SetCollisionEnabled(ECollisionEnabled::NoCollision);

        CollisionComponentPool.Add(PMC);
    }
}

UProceduralMeshComponent* UVoxelTerrainComponent::AcquireCollisionComponent()
{
    if (CollisionComponentPool.Num() == 0)
        return nullptr;

    UProceduralMeshComponent* PMC = CollisionComponentPool.Pop();
    PMC->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    return PMC;
}

void UVoxelTerrainComponent::ReturnCollisionComponent(UProceduralMeshComponent* PMC)
{
    if (!IsValid(PMC)) return;

    PMC->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    PMC->ClearAllMeshSections();
    CollisionComponentPool.Add(PMC);
}

// ============================================================
// ProcessCollisionQueue（自然场景的碰撞回收 + 创建）
//    编辑触发的碰撞更新已在 RebuildTouchedChunks 里同步处理，不走这里
// ============================================================
void UVoxelTerrainComponent::ProcessCollisionQueue()
{
    // ============ 0. 碰撞关闭时全清 ============
    if (!bGenerateCollision)
    {
        for (auto It = ChunkCollisionProxies.CreateIterator(); It; ++It)
        {
            if (UProceduralMeshComponent* PMC = It.Value().Get())
            {
                ReturnCollisionComponent(PMC);
            }
            It.RemoveCurrent();
        }
        return;
    }

    UWorld* World = GetWorld();
    if (!World) return;
    APlayerController* PC = World->GetFirstPlayerController();
    if (!PC) return;
    APawn* Pawn = PC->GetPawnOrSpectator();
    if (!Pawn) return;

    const FIntVector PlayerChunk = WorldToBaseChunk(Pawn->GetActorLocation());

    // ============ 1. 回收：不在半径内的 PMC 归还池子 ============
    for (auto It = ChunkCollisionProxies.CreateIterator(); It; ++It)
    {
        const FChunkKey& K = It.Key();
        const int32 dx = FMath::Abs(K.X - PlayerChunk.X);
        const int32 dy = FMath::Abs(K.Y - PlayerChunk.Y);
        const int32 dz = FMath::Abs(K.Z - PlayerChunk.Z);

        if (dx > CollisionRadius.X || dy > CollisionRadius.Y || dz > CollisionRadius.Z)
        {
            if (UProceduralMeshComponent* PMC = It.Value().Get())
            {
                ReturnCollisionComponent(PMC);
            }
            It.RemoveCurrent();
        }
    }

    // ============ 2. 创建：三轴双循环直接生成 KEY ============
    for (int32 dx = -CollisionRadius.X; dx <= CollisionRadius.X; ++dx)
    {
        for (int32 dy = -CollisionRadius.Y; dy <= CollisionRadius.Y; ++dy)
        {
            for (int32 dz = -CollisionRadius.Z; dz <= CollisionRadius.Z; ++dz)
            {
                FChunkKey Key(PlayerChunk.X + dx, PlayerChunk.Y + dy, PlayerChunk.Z + dz, 0);

                if (ChunkCollisionProxies.Contains(Key)) continue;

                auto* MeshPtr = ChunkMeshes.Find(Key);
                if (!MeshPtr || !MeshPtr->IsValid()) continue;
                const FVoxelMeshData& Mesh = **MeshPtr;
                if (Mesh.Positions.Num() == 0 || Mesh.Indices.Num() == 0) continue;

                UProceduralMeshComponent* PMC = AcquireCollisionComponent();
                if (!PMC) return;

                const int32 NumVerts = Mesh.Positions.Num();
                TArray<FVector> Verts;
                Verts.SetNumUninitialized(NumVerts);
                for (int32 i = 0; i < NumVerts; ++i)
                    Verts[i] = FVector(Mesh.Positions[i]);

                TArray<int32> Tris;
                Tris.SetNumUninitialized(Mesh.Indices.Num());
                for (int32 i = 0; i < Mesh.Indices.Num(); ++i)
                    Tris[i] = (int32)Mesh.Indices[i];

                PMC->CreateMeshSection_LinearColor(
                    0, Verts, Tris,
                    TArray<FVector>(), TArray<FVector2D>(),
                    TArray<FLinearColor>(), TArray<FProcMeshTangent>(),
                    true, true);

                ChunkCollisionProxies.Add(Key, PMC);
            }
        }
    }
}

// ============================================================
// 单个 chunk 碰撞移除
// ============================================================
void UVoxelTerrainComponent::RemoveCollisionForChunk(const FChunkKey& Key)
{
    UProceduralMeshComponent* PMC = ChunkCollisionProxies.FindRef(Key);
    if (IsValid(PMC))
    {
        ReturnCollisionComponent(PMC);
    }
    ChunkCollisionProxies.Remove(Key);
}

// ============================================================
// 清空所有碰撞
// ============================================================
void UVoxelTerrainComponent::ClearAllCollisions()
{
    for (auto& Pair : ChunkCollisionProxies)
    {
        if (UProceduralMeshComponent* PMC = Pair.Value.Get())
        {
            if (IsValid(PMC))
            {
                PMC->DestroyComponent();
            }
        }
    }
    ChunkCollisionProxies.Empty();

    for (UProceduralMeshComponent* PMC : CollisionComponentPool)
    {
        if (IsValid(PMC))
        {
            PMC->DestroyComponent();
        }
    }
    CollisionComponentPool.Empty();
}
void UVoxelTerrainComponent::RequestEdit(const FVector& WorldPos, float Radius, bool bIsAdd)
{
    //PendingEditTasks.Add(FVoxelEditTask{ WorldPos, Radius, bIsAdd });
    if (!PendingEditTasks.Enqueue(FVoxelEditTask{ WorldPos, Radius, bIsAdd }))
    {
        // 队列已满，可以记录日志或直接忽略这次编辑
    }

}

void UVoxelTerrainComponent::EditTaskTick()
{
    constexpr int32 MaxEditsPerFrame = 1; // 每帧处理上限

    FVoxelEditTask Task;
    int32 Count = 0;
    // 循环出队，直到达到每帧上限或队列为空
    while (Count < MaxEditsPerFrame && PendingEditTasks.Dequeue(Task))
    {
        ApplyEdit(Task.WorldPos, Task.Radius, Task.bIsAdd);
        ++Count;
    }

}

// ============================================================
// 表面植被系统
// ============================================================
void UVoxelTerrainComponent::InitBiomeTexture()
{
    CachedBiomeData.Reset();

    if (!BiomeTexture)
    {
        UE_LOG(LogTemp, Warning, TEXT("UVoxelTerrainComponent: BiomeTexture is null."));
        return;
    }

    FImage Img;
    if (BiomeTexture->Source.GetMipImage(Img, 0))
    {
        const int32 Size = BiomeTexture->GetSizeX();
        CachedBiomeData.SetNumUninitialized(Size * Size);

        const uint8* P = Img.RawData.GetData();

        if (Img.Format == ERawImageFormat::G8)
        {
            for (int32 i = 0; i < CachedBiomeData.Num(); ++i)
                CachedBiomeData[i] = P[i] / 255.f;
        }
        else if (Img.Format == ERawImageFormat::BGRA8)
        {
            // 取 R 通道（对齐 HLSL 的 .r）
            for (int32 i = 0; i < CachedBiomeData.Num(); ++i)
                CachedBiomeData[i] = P[i * 4 + 2] / 255.f;
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("UVoxelTerrainComponent: Unsupported BiomeTexture format."));
            return;
        }

        UE_LOG(LogTemp, Log, TEXT("UVoxelTerrainComponent: BiomeTexture initialized. Size = %d"), Size);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("UVoxelTerrainComponent: Failed to read BiomeTexture pixels."));
    }
}



void UVoxelTerrainComponent::DispatchVegetationTask(FChunkKey Key, TSharedPtr<FVoxelMeshData, ESPMode::ThreadSafe> Mesh)
{
    // ⭐ 如果没配贴图，或者没填配置表，直接退出
    if (!Mesh.IsValid() || !BiomeTexture || CachedBiomeData.Num() == 0 || VegetationConfigs.Num() == 0) return;

    TWeakObjectPtr<UVoxelTerrainComponent> WeakThis(this);

    const int32 LocalSize = BiomeTexture->GetSizeX();
    //const TArray<float> LocalData = CachedBiomeData;
    //const TArray<FVoxelVegetationConfig> LocalConfigs = VegetationConfigs; // ⭐ 拷贝配置
    const float WorldSize = 100000.f;
    Async(EAsyncExecution::ThreadPool, [WeakThis, Key, Mesh, LocalSize, WorldSize]()
        {
            const UVoxelTerrainComponent* Self = WeakThis.Get();
            if (!Self) return;

            // ⭐ 传入配置表
            TArray<FVegetationInstanceData> Results = Self->CalculateVegetationForChunk(
                *Mesh, Key, Self->CachedBiomeData, LocalSize, WorldSize, Self->VegetationConfigs
            );
            AsyncTask(ENamedThreads::GameThread, [WeakThis, Results = MoveTemp(Results)]()
                {
                    if (auto* MainSelf = WeakThis.Get())
                    {
                        if (UWorld* World = MainSelf->GetWorld())
                        {
          

                        }
                    }
                });
        });

}
// ⭐ 独立出来的纯计算函数：无副作用，仅做数学计算
TArray<FVegetationInstanceData> UVoxelTerrainComponent::CalculateVegetationForChunk(
    const FVoxelMeshData& Mesh,
    const FChunkKey& Key,
    const TArray<float>& LocalData,
    int32 LocalSize,
    float WorldSize,
    const TArray<FVoxelVegetationConfig>& Configs) const // ⭐ 接收配置表
{
    const int32 NumVerts = Mesh.Positions.Num();
    TArray<FVegetationInstanceData> FinalResults;
    if (NumVerts == 0 || Configs.Num() == 0) return FinalResults;

    const float InvWorldSize = LocalSize / WorldSize;

    struct FHitData { FVector Location; int32 ConfigIndex; float Radius; };
    TArray<FHitData> Hits;
    Hits.Reserve(NumVerts / 20);

    const uint32 BaseSeed = HashCombine(HashCombine(Key.X, Key.Y), Key.Z);

    for (int32 i = 0; i < NumVerts; ++i)
    {
        // 1. 法线过滤
        if (Mesh.Normals[i].Z <= 0.5f) continue;

        const FVector Pos = FVector(Mesh.Positions[i]);

        // 2. 采样群系贴图
        float FX = Pos.X * InvWorldSize;
        float FY = Pos.Y * InvWorldSize;

        int32 X0 = FMath::FloorToInt(FX) % LocalSize; if (X0 < 0) X0 += LocalSize;
        int32 Y0 = FMath::FloorToInt(FY) % LocalSize; if (Y0 < 0) Y0 += LocalSize;
        int32 X1 = (X0 + 1) % LocalSize;
        int32 Y1 = (Y0 + 1) % LocalSize;

        const float tx = FX - FMath::FloorToFloat(FX);
        const float ty = FY - FMath::FloorToFloat(FY);

        const float Gray = FMath::Lerp(
            FMath::Lerp(LocalData[Y0 * LocalSize + X0], LocalData[Y0 * LocalSize + X1], tx),
            FMath::Lerp(LocalData[Y1 * LocalSize + X0], LocalData[Y1 * LocalSize + X1], tx),
            ty
        );

        const float ID = FMath::FloorToFloat(Gray * 255.f + 0.5f);

        // 3. ⭐ 完全数据驱动：遍历你的配置
        for (int32 c = 0; c < Configs.Num(); ++c)
        {
            const auto& Config = Configs[c];

            // 当前噪点 ID 是否在这个配置的区间内
            if (ID >= Config.MinID && ID <= Config.MaxID)
            {
                // 用确定性随机数做概率判定
                uint32 Seed = HashCombine(BaseSeed, i * 1000 + c);
                Seed ^= Seed >> 16; Seed *= 0x7feb352dU;
                Seed ^= Seed >> 15; Seed *= 0x846ca68bU;
                Seed ^= Seed >> 16;
                float RandVal = (Seed & 0xFFFFFF) * (1.0f / 16777216.0f);

                if (RandVal < Config.Density)
                {
                    Hits.Add({ Pos, c, Config.PlaceRadius });
                }
            }
        }
    }

    if (Hits.Num() == 0) return FinalResults;

    // 4. 防重叠排序（半径大的先占位）
    Hits.Sort([](const FHitData& A, const FHitData& B) {
        return A.Radius > B.Radius;
        });

    // 5. 空间哈希网格防重叠
    const float CellSize = 200.f;
    TMap<FIntPoint, TArray<int32>> Grid;
    FinalResults.Reserve(Hits.Num());

    for (const auto& Hit : Hits)
    {
        const FIntPoint Cell(FMath::FloorToInt(Hit.Location.X / CellSize), FMath::FloorToInt(Hit.Location.Y / CellSize));
        bool bOverlap = false;

        for (int32 dx = -1; dx <= 1 && !bOverlap; ++dx)
        {
            for (int32 dy = -1; dy <= 1 && !bOverlap; ++dy)
            {
                if (const TArray<int32>* Nearby = Grid.Find(FIntPoint(Cell.X + dx, Cell.Y + dy)))
                {
                    for (int32 Idx : *Nearby)
                    {
                        const auto& Existing = FinalResults[Idx];
                        const float MinDist = Configs[Hit.ConfigIndex].PlaceRadius + Configs[Existing.PlantId].PlaceRadius;

                        if (FVector::DistSquared2D(Existing.Location, Hit.Location) < MinDist * MinDist)
                        {
                            bOverlap = true;
                            break;
                        }
                    }
                }
            }
        }

        if (!bOverlap)
        {
            FVegetationInstanceData NewInst;
            NewInst.Location = Hit.Location;
            NewInst.PlantId = Hit.ConfigIndex; // 记录配置索引

            // 6. 随机缩放和旋转
            uint32 ScaleSeed = HashCombine(BaseSeed, (int32)(Hit.Location.X + Hit.Location.Y));
            ScaleSeed ^= ScaleSeed >> 16; ScaleSeed *= 0x7feb352dU;
            float RandScale = (ScaleSeed & 0xFFFFFF) * (1.0f / 16777216.0f);

            const auto& Config = Configs[Hit.ConfigIndex];
            float FinalScale = FMath::Lerp(Config.MinScale, Config.MaxScale, RandScale);

            NewInst.Scale = FVector(FinalScale);
            NewInst.Rotation = FRotator(0.f, RandScale * 360.f, 0.f);

            int32 NewIdx = FinalResults.Add(NewInst);
            Grid.FindOrAdd(Cell).Add(NewIdx);
        }
    }

    return FinalResults;
}

// 植被系统 HISM 对象池逻辑

void UVoxelTerrainComponent::PrewarmHISMPool(int32 PoolSize)
{
    if (PoolSize <= 0) return;
    HISMPool.Reserve(PoolSize);

    for (int32 i = 0; i < PoolSize; ++i)
    {
        UHierarchicalInstancedStaticMeshComponent* HISM = NewObject<UHierarchicalInstancedStaticMeshComponent>(this, NAME_None, RF_Transient);
        if (!HISM) continue;

        HISM->SetupAttachment(this);
        HISM->RegisterComponent();
        HISM->SetMobility(EComponentMobility::Movable);
        HISM->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        HISM->bAutoRebuildTreeOnInstanceChanges = false; // 批量添加时提升性能
        HISM->SetVisibility(false); // 先隐藏，用到再开

        HISMPool.Add(HISM);
    }
}

UHierarchicalInstancedStaticMeshComponent* UVoxelTerrainComponent::AcquireHISM(UStaticMesh* Mesh)
{
    if (HISMPool.Num() == 0)
    {
        // 如果池子空了，备用动态扩容（确保不会崩）
        UHierarchicalInstancedStaticMeshComponent* NewHISM = NewObject<UHierarchicalInstancedStaticMeshComponent>(this, NAME_None, RF_Transient);
        NewHISM->SetupAttachment(this);
        NewHISM->RegisterComponent();
        NewHISM->SetMobility(EComponentMobility::Movable);
        NewHISM->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        NewHISM->bAutoRebuildTreeOnInstanceChanges = false;
        return NewHISM;
    }

    UHierarchicalInstancedStaticMeshComponent* HISM = HISMPool.Pop();
    HISM->ClearInstances();
    HISM->SetStaticMesh(Mesh);
    HISM->SetVisibility(true);
    return HISM;
}

void UVoxelTerrainComponent::ReturnHISM(UHierarchicalInstancedStaticMeshComponent* HISM)
{
    if (!IsValid(HISM)) return;

    HISM->ClearInstances(); // 清空所有实例
    HISM->SetVisibility(false);
    HISM->SetStaticMesh(nullptr);
    HISMPool.Add(HISM); // 放回池子
}