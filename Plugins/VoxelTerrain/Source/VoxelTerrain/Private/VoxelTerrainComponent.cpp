#include "VoxelTerrainComponent.h"
#include "VoxelSceneProxy.h"
#include "VoxelGenerator.h"
#include "VoxelChunkData.h"
#include "TransvoxelMesher.h"
#include "RenderingThread.h"
#include "Materials/Material.h"
#include "MaterialDomain.h"
#include "Async/Async.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "HAL/PlatformTime.h"
#include "Engine/Texture2D.h"
#include "ImageUtils.h"
#include "ProceduralMeshComponent.h"
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

    LoadNoiseTextureData();
    PrewarmCollisionPool(256);

    ChunkMeshes.Reserve(1024);
    ChunkLastActiveTime.Reserve(1024);
    RequestedChunks.Reserve(1024);
    PendingPushToSceneProxy.Reserve(64);
    PendingRebuilds.Reserve(64);
    ChunkVersions.Reserve(1024);
    ChunkModifications.Reserve(1024);



    TickHandle = FTSTicker::GetCoreTicker().AddTicker(
        FTickerDelegate::CreateWeakLambda(this, [this](float)
            {
                StreamTick();
                return true;
            }), 0.16f);
    CollisionTickHandle = FTSTicker::GetCoreTicker().AddTicker(
        FTickerDelegate::CreateWeakLambda(this, [this](float)
            {
                // 主线程处理
                ProcessCollisionQueue();
                return true;
            }), 0.33f);
}

void UVoxelTerrainComponent::EndPlay(const EEndPlayReason::Type Reason)
{
    if (TickHandle.IsValid())
        FTSTicker::GetCoreTicker().RemoveTicker(TickHandle);
    TickHandle.Reset();

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
// 修改数据收集（纯内存）
// ============================================================


// ============================================================
// Chunk 异步生成
// ============================================================
void UVoxelTerrainComponent::SpawnChunkAsyncTask(const FChunkKey& Key, int32 CurrentVersion)
{
    TWeakObjectPtr<UVoxelTerrainComponent> WeakThis(this);

    Async(EAsyncExecution::ThreadPool, [WeakThis, Key, CurrentVersion]()
        {
            FVoxelGenerator Gen;
            FVoxelChunkData ChunkData;
            Gen.GenerateChunk(Key.X, Key.Y, Key.Z, Key.LOD, ChunkData.Voxels.GetData());

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

            AsyncTask(ENamedThreads::GameThread, [WeakThis, Key, MeshData, CurrentVersion]()
                {
                    if (auto* Self = WeakThis.Get())
                        Self->OnChunkGenerated(Key, MeshData, CurrentVersion);
                });
        });
}

// ============================================================
// StreamTick
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

    // 重建队列
    constexpr int32 MaxRebuildsPerTick = 6;
    if (PendingRebuilds.Num() > 0)
    {
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
                PendingRebuilds.Remove(Key);
                continue;
            }
            RequestedChunks.Add(Key);
            const int32 CurrentVersion = ChunkVersions.FindRef(Key);
            SpawnChunkAsyncTask(Key, CurrentVersion);
            ++Submitted;
        }
    }

    // 收集加载候选
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

    // 卸载
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
        ChunkLastActiveTime.Remove(Key);
        RequestedChunks.Remove(Key);
        PendingPushToSceneProxy.Remove(Key);
        PendingRebuilds.Remove(Key);
        ChunkVersions.Remove(Key);

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
// OnChunkGenerated
// ============================================================
void UVoxelTerrainComponent::OnChunkGenerated(
    FChunkKey Key, TSharedPtr<FVoxelMeshData, ESPMode::ThreadSafe> Mesh, int32 Version)
{

    if (!Mesh.IsValid()) return;

#if 1
    RequestedChunks.Remove(Key);

    if (Version != ChunkVersions.FindRef(Key))
    {
        if (PendingRebuilds.Contains(Key) && !RequestedChunks.Contains(Key))
        {
            RequestedChunks.Add(Key);
            PendingCollisionUpdates.Add(Key);
            SpawnChunkAsyncTask(Key, ChunkVersions.FindRef(Key));
        }
        return;
    }
#else
    if (PendingRebuilds.Contains(Key) )
    {
        RequestedChunks.Add(Key);
        PendingCollisionUpdates.Add(Key);
        SpawnChunkAsyncTask(Key, ChunkVersions.FindRef(Key));
        return;
    }
#endif


    ChunkMeshes.Add(Key, Mesh);
    ChunkLastActiveTime.Add(Key, FPlatformTime::Seconds());
    PendingRebuilds.Remove(Key);

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
// ============================================================
void UVoxelTerrainComponent::ApplyEdit(const FVector& WorldPos, float Radius, bool bIsAdd)
{
    const float VoxelSize = VoxelConfig::VoxelSize;
    const FVector GridPos = WorldPos / VoxelSize;

    const float Scale = VoxelSize * 1.5f;
    const int32 RadiusInVoxels = FMath::CeilToInt((Radius + Scale) / VoxelSize);
    const float SearchRadius = Radius + Scale;
    const float SearchRadiusSq = SearchRadius * SearchRadius;

    const FIntVector MinGrid(FMath::FloorToInt(GridPos.X - RadiusInVoxels - 1),
        FMath::FloorToInt(GridPos.Y - RadiusInVoxels - 1),
        FMath::FloorToInt(GridPos.Z - RadiusInVoxels - 1));
    const FIntVector MaxGrid(FMath::FloorToInt(GridPos.X + RadiusInVoxels + 1),
        FMath::FloorToInt(GridPos.Y + RadiusInVoxels + 1),
        FMath::FloorToInt(GridPos.Z + RadiusInVoxels + 1));

    const float BaseChunkSize = VoxelConfig::ChunkSize * VoxelConfig::VoxelSize;
    FVoxelGenerator Gen;

    TSet<FChunkKey> TouchedChunks;
    TouchedChunks.Reserve(16);

    {
        // FScopeLock Lock(&DataLock);

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

                    const int32 LocalX = x - ChunkCoord.X * VoxelConfig::ChunkSize;
                    const int32 LocalY = y - ChunkCoord.Y * VoxelConfig::ChunkSize;
                    const int32 LocalZ = z - ChunkCoord.Z * VoxelConfig::ChunkSize;

                    const bool bNegX = (LocalX == 0);
                    const bool bNegY = (LocalY == 0);
                    const bool bNegZ = (LocalZ == 0);

                    TouchedChunks.Add(LOD0Key);

                    if (bNegX) TouchedChunks.Add(FChunkKey(ChunkCoord.X - 1, ChunkCoord.Y, ChunkCoord.Z, 0));
                    if (bNegY) TouchedChunks.Add(FChunkKey(ChunkCoord.X, ChunkCoord.Y - 1, ChunkCoord.Z, 0));
                    if (bNegZ) TouchedChunks.Add(FChunkKey(ChunkCoord.X, ChunkCoord.Y, ChunkCoord.Z - 1, 0));

                    if (bNegX && bNegY) TouchedChunks.Add(FChunkKey(ChunkCoord.X - 1, ChunkCoord.Y - 1, ChunkCoord.Z, 0));
                    if (bNegX && bNegZ) TouchedChunks.Add(FChunkKey(ChunkCoord.X - 1, ChunkCoord.Y, ChunkCoord.Z - 1, 0));
                    if (bNegY && bNegZ) TouchedChunks.Add(FChunkKey(ChunkCoord.X, ChunkCoord.Y - 1, ChunkCoord.Z - 1, 0));
                    if (bNegX && bNegY && bNegZ) TouchedChunks.Add(FChunkKey(ChunkCoord.X - 1, ChunkCoord.Y - 1, ChunkCoord.Z - 1, 0));
                }
            }
        }
    }

    if (TouchedChunks.Num() == 0) 
        return;

    RebuildTouchedChunks(TouchedChunks);
}

// ============================================================
// Rebuild / Immediate
// ============================================================
void UVoxelTerrainComponent::RebuildTouchedChunks(const TSet<FChunkKey>& TouchedChunks)
{
    for (const FChunkKey& Key : TouchedChunks)
    {
        PendingRebuilds.Add(Key);

        int32& Ver = ChunkVersions.FindOrAdd(Key);
        Ver++;

        if (ChunkMeshes.Contains(Key) && !RequestedChunks.Contains(Key))
        {
            RequestedChunks.Add(Key);
            SpawnChunkAsyncTask(Key, Ver);
        }
    }
}


// ============================================================
// 碰撞组件池
// ============================================================
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
    {
        return nullptr;
    }

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
// 碰撞核心
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
                if (!PMC) return;   // 池空，本帧结束

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


void UVoxelTerrainComponent::RemoveCollisionForChunk(const FChunkKey& Key)
{
    UProceduralMeshComponent* PMC = ChunkCollisionProxies.FindRef(Key);
    if (IsValid(PMC))
    {
        ReturnCollisionComponent(PMC);
    }
    ChunkCollisionProxies.Remove(Key);
}

void UVoxelTerrainComponent::ClearAllCollisions()
{
    for (auto& Pair : ChunkCollisionProxies)
    {
        if (IsValid(Pair.Value))
        {
            ReturnCollisionComponent(Pair.Value);
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