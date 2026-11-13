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
    ChunkModifications.Reserve(1024);
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
    FVoxelModificationMap ModsSnapshot;
    if (ChunkModifications.Num() > 0)
    {
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

        for (int32 x = MinX; x <= MaxX; ++x)
            for (int32 y = MinY; y <= MaxY; ++y)
                for (int32 z = MinZ; z <= MaxZ; ++z)
                    if (const FVoxelModificationMap* M = ChunkModifications.Find(FChunkKey(x, y, z, 0)))
                        ModsSnapshot.Append(*M);
    }

    TWeakObjectPtr<UVoxelTerrainComponent> WeakThis(this);

    Async(EAsyncExecution::ThreadPool,
        [WeakThis, Key, CurrentVersion, ModsSnapshot = MoveTemp(ModsSnapshot)]()
        {
            FVoxelGenerator Gen;
            FVoxelChunkData ChunkData;
            Gen.GenerateChunk(Key.X, Key.Y, Key.Z, Key.LOD, ModsSnapshot, /*ChunkData.Voxels.GetData()*/ ChunkData);

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
    const float VoxelSize = VoxelConfig::VoxelSize;
    const FVector GridPos = WorldPos / VoxelSize;

    const float Scale = VoxelSize * 1.5f;
    const int32 RadiusInVoxels = FMath::CeilToInt((Radius + Scale) / VoxelSize);
    const float SearchRadius = Radius + Scale;
    const float SearchRadiusSq = SearchRadius * SearchRadius;

    const FIntVector MinGrid(
        FMath::FloorToInt(GridPos.X - RadiusInVoxels - 1),
        FMath::FloorToInt(GridPos.Y - RadiusInVoxels - 1),
        FMath::FloorToInt(GridPos.Z - RadiusInVoxels - 1));
    const FIntVector MaxGrid(
        FMath::FloorToInt(GridPos.X + RadiusInVoxels + 1),
        FMath::FloorToInt(GridPos.Y + RadiusInVoxels + 1),
        FMath::FloorToInt(GridPos.Z + RadiusInVoxels + 1));

    const float BaseChunkSize = VoxelConfig::ChunkSize * VoxelConfig::VoxelSize;
    constexpr int32 ChunkN = VoxelConfig::ChunkSize;    // 32
    constexpr int32 MaxHalo = ChunkN + 1;               // 33

    TSet<FChunkKey> TouchedChunks;
    TouchedChunks.Reserve(16);

    // ⭐ 27 邻居缓存：只在 ChunkCoord 变化时刷新
    FIntVector LastChunkCoord(INT32_MIN, INT32_MIN, INT32_MIN);
    FVoxelChunkData* NeighborData[27] = {};
    FVoxelModificationMap* MainMods = nullptr;
    FIntVector MainChunkOriginVoxel(0, 0, 0);

    for (int32 x = MinGrid.X; x <= MaxGrid.X; ++x) {
        for (int32 y = MinGrid.Y; y <= MaxGrid.Y; ++y) {
            for (int32 z = MinGrid.Z; z <= MaxGrid.Z; ++z) {
                const FVector VoxelWorldPos(x * VoxelSize, y * VoxelSize, z * VoxelSize);
                const float DistSq = FVector::DistSquared(VoxelWorldPos, WorldPos);
                if (DistSq > SearchRadiusSq) continue;

                const float Dist = FMath::Sqrt(DistSq);
                const float SDF = bIsAdd ? (Dist - Radius) : (Radius - Dist);
                const float W = FMath::Clamp(SDF / Scale, -1.f, 1.f);
                const int8 SDFWeight = (FVoxelWeight)FMath::RoundToInt(W * 127.f);

                const FIntVector ChunkCoord(
                    FMath::FloorToInt((x * VoxelSize) / BaseChunkSize),
                    FMath::FloorToInt((y * VoxelSize) / BaseChunkSize),
                    FMath::FloorToInt((z * VoxelSize) / BaseChunkSize));
                const FChunkKey LOD0Key(ChunkCoord.X, ChunkCoord.Y, ChunkCoord.Z, 0);

                // ⭐ 只在小 chunk 变化时刷新 27 邻居指针
                if (ChunkCoord != LastChunkCoord)
                {
                    LastChunkCoord = ChunkCoord;
                    MainChunkOriginVoxel = FIntVector(
                        ChunkCoord.X * ChunkN,
                        ChunkCoord.Y * ChunkN,
                        ChunkCoord.Z * ChunkN);

                    int32 idx = 0;
                    for (int32 cdx = -1; cdx <= 1; ++cdx)
                        for (int32 cdy = -1; cdy <= 1; ++cdy)
                            for (int32 cdz = -1; cdz <= 1; ++cdz)
                            {
                                const FChunkKey NK(
                                    ChunkCoord.X + cdx,
                                    ChunkCoord.Y + cdy,
                                    ChunkCoord.Z + cdz,
                                    0);
                                NeighborData[idx] = nullptr;
                                if (auto* P = ChunkVoxelData.Find(NK))
                                    if (P->IsValid()) NeighborData[idx] = P->Get();
                                ++idx;
                            }

                    // 主 chunk（idx == 13）已加载 → 才处理
                    MainMods = NeighborData[13]
                        ? &ChunkModifications.FindOrAdd(LOD0Key)
                        : nullptr;
                }

                // ⭐ 主 chunk 没加载 → 不反应（不做兜底）
                if (!MainMods) continue;

                // ① 读当前值：优先 Mods，其次从主 chunk 缓存读
                const FIntVector VoxelCoord(x, y, z);
                int8 CurrentWeight;
                if (const FVoxelWeight* Existing = MainMods->Find(VoxelCoord))
                {
                    CurrentWeight = *Existing;
                }
                else
                {
                    const int32 LX = x - MainChunkOriginVoxel.X;
                    const int32 LY = y - MainChunkOriginVoxel.Y;
                    const int32 LZ = z - MainChunkOriginVoxel.Z;
                    const int32 PaddedIdx = NeighborData[13]->Index(LX, LY, LZ);
                    CurrentWeight = NeighborData[13]->Voxels[PaddedIdx].Weight;
                }

                const int8 FinalWeight = bIsAdd
                    ? FMath::Min<int8>(CurrentWeight, SDFWeight)
                    : FMath::Max<int8>(CurrentWeight, SDFWeight);

                // ② 持久化到 ChunkModifications
                MainMods->Add(VoxelCoord, FinalWeight);

                // ③ 原地写 27 邻居 halo（只写已加载的）
                {
                    int32 idx = 0;
                    for (int32 cdx = -1; cdx <= 1; ++cdx)
                        for (int32 cdy = -1; cdy <= 1; ++cdy)
                            for (int32 cdz = -1; cdz <= 1; ++cdz)
                            {
                                FVoxelChunkData* Data = NeighborData[idx];
                                if (Data)
                                {
                                    const int32 LX = x - (MainChunkOriginVoxel.X + cdx * ChunkN);
                                    const int32 LY = y - (MainChunkOriginVoxel.Y + cdy * ChunkN);
                                    const int32 LZ = z - (MainChunkOriginVoxel.Z + cdz * ChunkN);

                                    if (LX >= -1 && LX <= MaxHalo &&
                                        LY >= -1 && LY <= MaxHalo &&
                                        LZ >= -1 && LZ <= MaxHalo)
                                    {
                                        Data->At(LX, LY, LZ).Weight = FinalWeight;
                                    }
                                }
                                ++idx;
                            }
                }

                // ④ 标记受影响的 chunk（halo 边界：LocalX <= 1 或 >= ChunkN-1）
                TouchedChunks.Add(LOD0Key);

                const int32 LocalX = x - MainChunkOriginVoxel.X;
                const int32 LocalY = y - MainChunkOriginVoxel.Y;
                const int32 LocalZ = z - MainChunkOriginVoxel.Z;

                const bool bNegX = (LocalX <= 1);
                const bool bPosX = (LocalX >= ChunkN - 1);
                const bool bNegY = (LocalY <= 1);
                const bool bPosY = (LocalY >= ChunkN - 1);
                const bool bNegZ = (LocalZ <= 1);
                const bool bPosZ = (LocalZ >= ChunkN - 1);

                if (bNegX) TouchedChunks.Add(FChunkKey(ChunkCoord.X - 1, ChunkCoord.Y, ChunkCoord.Z, 0));
                if (bPosX) TouchedChunks.Add(FChunkKey(ChunkCoord.X + 1, ChunkCoord.Y, ChunkCoord.Z, 0));
                if (bNegY) TouchedChunks.Add(FChunkKey(ChunkCoord.X, ChunkCoord.Y - 1, ChunkCoord.Z, 0));
                if (bPosY) TouchedChunks.Add(FChunkKey(ChunkCoord.X, ChunkCoord.Y + 1, ChunkCoord.Z, 0));
                if (bNegZ) TouchedChunks.Add(FChunkKey(ChunkCoord.X, ChunkCoord.Y, ChunkCoord.Z - 1, 0));
                if (bPosZ) TouchedChunks.Add(FChunkKey(ChunkCoord.X, ChunkCoord.Y, ChunkCoord.Z + 1, 0));

                // 角组合（XY / XZ / YZ）
                if (bNegX && bNegY) TouchedChunks.Add(FChunkKey(ChunkCoord.X - 1, ChunkCoord.Y - 1, ChunkCoord.Z, 0));
                if (bNegX && bPosY) TouchedChunks.Add(FChunkKey(ChunkCoord.X - 1, ChunkCoord.Y + 1, ChunkCoord.Z, 0));
                if (bPosX && bNegY) TouchedChunks.Add(FChunkKey(ChunkCoord.X + 1, ChunkCoord.Y - 1, ChunkCoord.Z, 0));
                if (bPosX && bPosY) TouchedChunks.Add(FChunkKey(ChunkCoord.X + 1, ChunkCoord.Y + 1, ChunkCoord.Z, 0));
                if (bNegX && bNegZ) TouchedChunks.Add(FChunkKey(ChunkCoord.X - 1, ChunkCoord.Y, ChunkCoord.Z - 1, 0));
                if (bNegX && bPosZ) TouchedChunks.Add(FChunkKey(ChunkCoord.X - 1, ChunkCoord.Y, ChunkCoord.Z + 1, 0));
                if (bPosX && bNegZ) TouchedChunks.Add(FChunkKey(ChunkCoord.X + 1, ChunkCoord.Y, ChunkCoord.Z - 1, 0));
                if (bPosX && bPosZ) TouchedChunks.Add(FChunkKey(ChunkCoord.X + 1, ChunkCoord.Y, ChunkCoord.Z + 1, 0));
                if (bNegY && bNegZ) TouchedChunks.Add(FChunkKey(ChunkCoord.X, ChunkCoord.Y - 1, ChunkCoord.Z - 1, 0));
                if (bNegY && bPosZ) TouchedChunks.Add(FChunkKey(ChunkCoord.X, ChunkCoord.Y - 1, ChunkCoord.Z + 1, 0));
                if (bPosY && bNegZ) TouchedChunks.Add(FChunkKey(ChunkCoord.X, ChunkCoord.Y + 1, ChunkCoord.Z - 1, 0));
                if (bPosY && bPosZ) TouchedChunks.Add(FChunkKey(ChunkCoord.X, ChunkCoord.Y + 1, ChunkCoord.Z + 1, 0));
            }
        }
    }

    if (TouchedChunks.Num() == 0) return;

    RebuildTouchedChunks(TouchedChunks);
}


// ============================================================
// RebuildTouchedChunks
//   ⭐ 优化点：
//     1. 复用旧 FVoxelMeshData（不 MakeShared）
//     2. 直接从 ChunkVoxelData 读体素（跳过 GenerateChunk）
//     3. Verts/Tris 用 static thread_local 复用
//     4. UV 用乘法（/ 400 → * 0.0025）
// ============================================================


void UVoxelTerrainComponent::RebuildTouchedChunks(const TSet<FChunkKey>& TouchedChunks)
{
    // ⭐ 复用 Verts/Tris 数组，避免每 chunk 520KB malloc/free
    static thread_local TArray<FVector> VertsCache;
    static thread_local TArray<int32> TrisCache;

    constexpr float InvUVScale = 1.f / 400.f;

    for (const FChunkKey& Key : TouchedChunks)
    {
        ChunkVersions.FindOrAdd(Key)++;
        if (!ChunkMeshes.Contains(Key)) continue;

        // 体素缓存必须存在
        auto* CachedVoxels = ChunkVoxelData.Find(Key);
        if (!CachedVoxels || !CachedVoxels->IsValid()) continue;

        // 复用旧的 MeshData
        auto* ExistingMesh = ChunkMeshes.Find(Key);
        if (!ExistingMesh || !ExistingMesh->IsValid()) continue;

        FVoxelMeshData& MeshData = **ExistingMesh;

        const float S = (float)(1 << Key.LOD);
        const float LODChunkWorldSize = VoxelConfig::ChunkSize * VoxelConfig::VoxelSize * S;
        const FVector3f Offset(
            Key.X * LODChunkWorldSize,
            Key.Y * LODChunkWorldSize,
            Key.Z * LODChunkWorldSize);

        // ⭐ 直接用缓存体素生成网格（跳过 GenerateChunk）
        FTransvoxelMesher::GenerateRegular(
            **CachedVoxels, VoxelConfig::VoxelSize, Key.LOD, Offset, MeshData);

        // ════════════════════════════════════════════════════════════
        // ⭐ 一次循环搞定 UV + Bounds + VertsCache（界限 = NumVerts）
        // ════════════════════════════════════════════════════════════
        const int32 NumVerts = MeshData.Positions.Num();
        MeshData.UVs.SetNumUninitialized(NumVerts);

        VertsCache.Reset();
        VertsCache.SetNumUninitialized(NumVerts);

        FBox MeshBounds(ForceInit);
        for (int32 i = 0; i < NumVerts; ++i)
        {
            FVector3f& P = MeshData.Positions[i];
            P += Offset;
            MeshData.UVs[i] = FVector2f(P.X * InvUVScale, P.Y * InvUVScale);
            MeshBounds += FVector(P);
            VertsCache[i] = FVector(P);
        }
        MeshData.Bounds = MeshBounds;

        // ════════════════════════════════════════════════════════════
        // ⭐ TrisCache：memcpy 一次搞定（界限 = NumIndices）
        //    uint32 → int32 是 bit-identical，直接拷内存
        // ════════════════════════════════════════════════════════════
        TrisCache.Reset();
        TrisCache.SetNumUninitialized(MeshData.Indices.Num());
        if (MeshData.Indices.Num() > 0)
        {
            FMemory::Memcpy(
                TrisCache.GetData(),
                MeshData.Indices.GetData(),
                MeshData.Indices.Num() * sizeof(int32));
        }

        ChunkLastActiveTime.Add(Key, FPlatformTime::Seconds());

        if (MeshData.Bounds.IsValid)
        {
            if (LocalBounds.IsValid)
                LocalBounds += MeshData.Bounds;
            else
                LocalBounds = MeshData.Bounds;
        }
        bBoundsDirty = true;

        // 推渲染线程（复用同一个 SharedPtr）
        FVoxelSceneProxy* Proxy = static_cast<FVoxelSceneProxy*>(SceneProxy);

        if (Proxy)
        {
            //TSharedPtr<FVoxelMeshData, ESPMode::ThreadSafe> MeshPtr = *ExistingMesh;
            auto MeshPtr = MakeShared<FVoxelMeshData, ESPMode::ThreadSafe>(**ExistingMesh);
            ENQUEUE_RENDER_COMMAND(AddVoxelChunk)(
                [Proxy, Key, MeshPtr](FRHICommandListImmediate& RHICmdList)
                {
                    Proxy->AddChunk_RenderThread(RHICmdList, Key, MeshPtr);
                });
        }
        else
        {
            PendingPushToSceneProxy.Add(Key);
        }

        // 碰撞更新（复用 VertsCache/TrisCache）
        UProceduralMeshComponent* PMC = ChunkCollisionProxies.FindRef(Key).Get();
        if (IsValid(PMC))
        {
            if (MeshData.Positions.Num() == 0 || MeshData.Indices.Num() == 0)
            {
                ReturnCollisionComponent(PMC);
                ChunkCollisionProxies.Remove(Key);
            }
            else
            {
                PMC->CreateMeshSection_LinearColor(
                    0, VertsCache, TrisCache,
                    TArray<FVector>(), TArray<FVector2D>(),
                    TArray<FLinearColor>(), TArray<FProcMeshTangent>(),
                    true, false);
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
    PendingEditTasks.Add(FVoxelEditTask{ WorldPos, Radius, bIsAdd });
}

void UVoxelTerrainComponent::EditTaskTick()
{
    if (PendingEditTasks.Num() == 0) return;

    TArray<FVoxelEditTask> Tasks;
    Tasks.Reserve(PendingEditTasks.Num());
    for (const FVoxelEditTask& T : PendingEditTasks)
        Tasks.Add(T);
    PendingEditTasks.Reset();

    for (const FVoxelEditTask& T : Tasks)
        ApplyEdit(T.WorldPos, T.Radius, T.bIsAdd);
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