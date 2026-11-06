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
#include "Engine/Texture2D.h"
#include "ImageUtils.h"      
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
    ChunkMeshes.Reserve(1024);
    ChunkLastActiveTime.Reserve(1024);
    ChunkCollisionKeepTime.Reserve(128);
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

    PendingSaveCounter = MakeShared<FThreadSafeCounter>();

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

void UVoxelTerrainComponent::LoadNoiseTextureData()
{
#if VOXEL_USE_NOISE_TEXTURE

    // ============================================================
    // 1. 加载 2D 噪声贴图
    // ============================================================
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
                for (int32 i = 0; i < NumPixels; ++i) F[i] = P[i * 4 + 2] / 255.f; // 取 R 通道
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

            UE_LOG(LogTemp, Warning, TEXT("VoxelTerrain: 加载 2D 噪声图层 %dx%d (Freq: %.2f, Amp: %.2f)"),
                SX, SY, NewLayer.Frequency, NewLayer.Amplitude);
        }
    }

    FVoxelGenerator::GlobalNoiseLayers = NewLayers2D;
    if (NewLayers2D->Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("VoxelTerrain: 未配置 2D 噪声图层，地表将使用默认 Perlin。"));
    }

    // ============================================================
    // 2. 加载 3D 体积贴图
    // ============================================================
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
            // ⭐ 修复点：FImage 没有 SizeZ，必须从贴图的 Source 里拿
            const int32 SZ = LayerInfo.Texture->Source.GetVolumeSizeZ();
            if (SX != SY || SX != SZ || SX < 4)
            {
                UE_LOG(LogTemp, Warning, TEXT("VoxelTerrain: 3D 贴图必须 XYZ 尺寸一致 (当前 %dx%dx%d)"), SX, SY, SZ);
                continue;
            }

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

            UE_LOG(LogTemp, Warning, TEXT("VoxelTerrain: 加载 3D 噪声图层 %dx%dx%d (Freq: %.2f, Amp: %.2f)"),
                SX, SY, SZ, NewLayer.Frequency, NewLayer.Amplitude);
        }
    }

    FVoxelGenerator::GlobalNoiseLayers3D = NewLayers3D;
    if (NewLayers3D->Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("VoxelTerrain: 未配置 3D 噪声图层，洞穴将使用默认 Perlin。"));
    }

#endif
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

    const double StartTime = FPlatformTime::Seconds();
    while (PendingSaveCounter->GetValue() > 0)
    {
        if (FPlatformTime::Seconds() - StartTime > 10.0)
        {
            //UE_LOG(LogTemp, Warning, TEXT("VoxelTerrain: FlushAllPendingSaves 超时，仍有 %d 个保存未完成"), PendingSaveCounter->GetValue());
            break;
        }
        FPlatformProcess::Sleep(0.001f);
    }
}

void UVoxelTerrainComponent::LoadChunkModificationFile(const FChunkKey& Key)
{
    if (Key.LOD != 0) return;
    if (FileLoadedChunks.Contains(Key)) return;


    const FString FilePath = GetChunkFilePath(Key);
    if (!FPaths::FileExists(FilePath))
    {
        FileLoadedChunks.Add(Key);   // ⬅ 补上
        return;
    }

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
    FScopeLock Lock(&DataLock);
    for (int32 x = MinX; x <= MaxX; ++x) {
        for (int32 y = MinY; y <= MaxY; ++y) {
            for (int32 z = MinZ; z <= MaxZ; ++z) {
                FChunkKey SubKey(x, y, z, 0);
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
                ModsCopy = Self->GatherModificationsForChunk(Key);
            }

            FVoxelGenerator Gen;
            FVoxelChunkData ChunkData;
            Gen.GenerateChunk(Key.X, Key.Y, Key.Z, Key.LOD, ModsCopy, ChunkData.Voxels.GetData());

            auto MeshData = MakeShared<FVoxelMeshData, ESPMode::ThreadSafe>();
            //FTransvoxelMesher::GenerateRegular(ChunkData, VoxelConfig::VoxelSize, Key.LOD, *MeshData);

       
            const float S = (float)(1 << Key.LOD);
            const float LODChunkWorldSize = VoxelConfig::ChunkSize * VoxelConfig::VoxelSize * S;
            const FVector3f Offset(
                Key.X * LODChunkWorldSize,
                Key.Y * LODChunkWorldSize,
                Key.Z * LODChunkWorldSize);

            FTransvoxelMesher::GenerateRegular(
                ChunkData, VoxelConfig::VoxelSize, Key.LOD, Offset, *MeshData);   // ⭐ 多传一个 Offset

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

    struct FLoadCandidate
    {
        float     DistSq = 0.f;
        FChunkKey Key;
    };
    TArray<FLoadCandidate> Candidates;
    Candidates.Reserve(1024);

    const float BaseChunkSize = VoxelConfig::ChunkSize * VoxelConfig::VoxelSize;
    const FIntVector Base = WorldToBaseChunk(CamLoc);

    // ⭐ 按 Chunk 数量加载，不受 VoxelSize 影响
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

    // 卸载半径：多留一圈 Chunk 缓冲
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
        ChunkCollisionKeepTime.Remove(Key);
        RequestedChunks.Remove(Key);
        PendingPushToSceneProxy.Remove(Key);
        PendingRebuilds.Remove(Key);
        ChunkVersions.Remove(Key);
        RemoveCollisionForChunk(Key);

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

    //if (ToUnload.Num() > 0) bBoundsDirty = true;

    if (ToUnload.Num() > 0)
    {
        // ⭐ 重建 LocalBounds，避免只增不减
        LocalBounds = FBox(ForceInit);
        for (const auto& Pair : ChunkMeshes)
        {
            if (Pair.Value.IsValid() && Pair.Value->Bounds.IsValid)
                LocalBounds += Pair.Value->Bounds;
        }
        bBoundsDirty = true;
    }


    UpdateCollisions(CamLoc, Now);

    if (bBoundsDirty)
    {
        bBoundsDirty = false;
        UpdateBounds();
        MarkRenderTransformDirty();
    }
    if (ChunkCollisionKeepTime.Num() > 0)
    {
        for (auto It = ChunkCollisionKeepTime.CreateIterator(); It; ++It)
        {
            if (Now >= It.Value())    // ⭐ It.Value()，不是 It->Value()
                It.RemoveCurrent();
        }
    }
}

void UVoxelTerrainComponent::OnChunkGenerated(
    FChunkKey Key, TSharedPtr<FVoxelMeshData, ESPMode::ThreadSafe> Mesh, int32 Version)
{
    RequestedChunks.Remove(Key);

    if (!Mesh.IsValid()) return;

    if (Version != ChunkVersions.FindRef(Key))
    {
        if (PendingRebuilds.Contains(Key) && !RequestedChunks.Contains(Key))
        {
            RequestedChunks.Add(Key);
            SpawnChunkAsyncTask(Key, ChunkVersions.FindRef(Key));
        }
        return;
    }

    const bool bIsFirstTime = !ChunkMeshes.Contains(Key);
    ChunkMeshes.Add(Key, Mesh);
    ChunkLastActiveTime.Add(Key, FPlatformTime::Seconds());
    PendingRebuilds.Remove(Key);

  /*  if (bIsFirstTime && Mesh->Bounds.IsValid)
    {
        LocalBounds += Mesh->Bounds;
    }*/

    #if 1
    LocalBounds = FBox(ForceInit);
    for (auto& Pair : ChunkMeshes)
    {
        if (Pair.Value.IsValid() && Pair.Value->Bounds.IsValid)
            LocalBounds += Pair.Value->Bounds;
    }
    bBoundsDirty = true;
    #else
    if (Mesh.IsValid() && Mesh->Bounds.IsValid)
    {
        LocalBounds += Mesh->Bounds;
    }
    bBoundsDirty = true;
    #endif

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
        if (!PendingCollisionAdds.Contains(Key))
        {
            PendingCollisionAdds.Add(Key);
        }
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

    // ⭐ 直接使用 Chunk 索引判断距离，无视 VoxelSize
    const FIntVector PlayerChunk = WorldToBaseChunk(CamLoc);
    const int32 Radius = CollisionRadiusChunks;

    float PlayerVelZ = 0.f;
    if (UWorld* World = GetWorld())
    {
        if (APlayerController* PC = World->GetFirstPlayerController())
        {
            if (APawn* Pawn = PC->GetPawnOrSpectator()) PlayerVelZ = Pawn->GetVelocity().Z;
        }
    }
    const bool bFallingFast = PlayerVelZ < -1000.f;

    for (auto& Pair : ChunkMeshes)
    {
        const FChunkKey& Key = Pair.Key;
        if (Key.LOD != 0) continue;

        const int32 dx = FMath::Abs(Key.X - PlayerChunk.X);
        const int32 dy = FMath::Abs(Key.Y - PlayerChunk.Y);
        const int32 dz = FMath::Abs(Key.Z - PlayerChunk.Z);
        const bool bInRange = (dx <= Radius && dy <= Radius && dz <= Radius);

        bool bKeepAlive = false;
        if (const double* KeepUntil = ChunkCollisionKeepTime.Find(Key))
        {
            bKeepAlive = (Now < *KeepUntil);
            if (!bKeepAlive) ChunkCollisionKeepTime.Remove(Key);
        }

        bool bShouldHave = bInRange || bKeepAlive;

        if (bShouldHave && bFallingFast && !bKeepAlive)
        {
            if (!ChunkCollisionProxies.Contains(Key))
            {
                if (dz > 1) bShouldHave = false; // 快速下落时只加载脚下一层
            }
        }

        const bool bHas = ChunkCollisionProxies.Contains(Key);
        const bool bPendingAdd = PendingCollisionAdds.Contains(Key);
        const bool bPendingRemove = PendingCollisionRemoves.Contains(Key);

        if (bShouldHave && !bHas && !bPendingAdd) PendingCollisionAdds.Add(Key);
        else if (!bShouldHave && bHas && !bPendingRemove) PendingCollisionRemoves.Add(Key);
    }
}


void UVoxelTerrainComponent::ProcessCollisionQueue()
{
    // ==================== 移除队列（不变）====================
    int32 RemoveBudget = 2;
    while (PendingCollisionRemoves.Num() > 0 && RemoveBudget > 0)
    {
        FChunkKey Key = PendingCollisionRemoves.Pop();
        RemoveCollisionForChunk(Key);
        --RemoveBudget;
    }

    if (PendingCollisionAdds.Num() == 0) return;

    // ==================== VIP 优先（不变）====================
    TArray<FChunkKey> VIPKeys;
    for (auto It = PendingCollisionAdds.CreateIterator(); It; ++It)
    {
        if (ChunkCollisionKeepTime.Contains(*It))
        {
            VIPKeys.Add(*It);
            It.RemoveCurrent();
        }
    }

    for (const FChunkKey& Key : VIPKeys)
    {
        TSharedPtr<FVoxelMeshData, ESPMode::ThreadSafe>* MeshPtr = ChunkMeshes.Find(Key);
        if (MeshPtr && MeshPtr->IsValid()) CreateCollisionForChunk(Key, **MeshPtr);
    }

    if (PendingCollisionAdds.Num() == 0) return;

    // ==================== 玩家位置（只算一次）====================
    FVector PlayerLoc = FVector::ZeroVector;
    if (UWorld* World = GetWorld())
    {
        if (APlayerController* PC = World->GetFirstPlayerController())
        {
            if (APawn* Pawn = PC->GetPawnOrSpectator()) PlayerLoc = Pawn->GetActorLocation();
        }
    }
    const FIntVector PlayerChunk = WorldToBaseChunk(PlayerLoc);

    // ⭐ 优化 1：队列过长时截断（远处 Chunk 反正走不到，先丢弃最远的）
    constexpr int32 MaxPendingCollisions = 64;
    if (PendingCollisionAdds.Num() > MaxPendingCollisions)
    {
        PendingCollisionAdds.Sort([&PlayerChunk](const FChunkKey& A, const FChunkKey& B)
            {
                auto Dist = [&PlayerChunk](const FChunkKey& K) -> int32
                    {
                        const int32 dx = K.X - PlayerChunk.X;
                        const int32 dy = K.Y - PlayerChunk.Y;
                        const int32 dz = K.Z - PlayerChunk.Z;
                        return dx * dx + dy * dy + dz * dz;
                    };
                return Dist(A) > Dist(B); // 降序，最近的在尾部
            });
        PendingCollisionAdds.SetNum(MaxPendingCollisions, EAllowShrinking::No);
    }

    // ⭐ 优化 2：只在队列比预算大时才排序，否则反正都要处理完
    constexpr int32 AddBudget = 3;
    if (PendingCollisionAdds.Num() > AddBudget)
    {
        // ⭐ 核心优化：整数距离 + 零 FVector 构造
        // 降序排序 → Pop 时取尾部 = 最近的
        PendingCollisionAdds.Sort([&PlayerChunk](const FChunkKey& A, const FChunkKey& B)
            {
                auto Dist = [&PlayerChunk](const FChunkKey& K) -> int32
                    {
                        const int32 dx = K.X - PlayerChunk.X;
                        const int32 dy = K.Y - PlayerChunk.Y;
                        const int32 dz = K.Z - PlayerChunk.Z;
                        return dx * dx + dy * dy + dz * dz;
                    };
                return Dist(A) > Dist(B);
            });
    }

    // ==================== 处理（不变）====================
    const double StartTime = FPlatformTime::Seconds();
    constexpr double MaxTimeBudget = 0.003;
    int32 Budget = AddBudget;

    while (PendingCollisionAdds.Num() > 0 && Budget > 0)
    {
        if (FPlatformTime::Seconds() - StartTime > MaxTimeBudget) break;

        FChunkKey Key = PendingCollisionAdds.Pop();
        --Budget;

        TSharedPtr<FVoxelMeshData, ESPMode::ThreadSafe>* MeshPtr = ChunkMeshes.Find(Key);
        if (MeshPtr && MeshPtr->IsValid()) CreateCollisionForChunk(Key, **MeshPtr);
    }
}

void UVoxelTerrainComponent::CreateCollisionForChunk(const FChunkKey& Key, const FVoxelMeshData& Mesh)
{
    if (Mesh.Positions.Num() == 0 || Mesh.Indices.Num() == 0) return;

    const int32 NumVerts = Mesh.Positions.Num();
    TArray<FVector> Verts;
    TArray<int32> Tris;
    TArray<FVector> Normals;
    TArray<FVector2D> UVs;
    TArray<FLinearColor> Colors;
    TArray<FProcMeshTangent> Tangents;

    Verts.SetNumUninitialized(NumVerts);
    //Normals.SetNumUninitialized(NumVerts);
    //UVs.SetNumUninitialized(NumVerts);
    //Colors.SetNumUninitialized(NumVerts);
    //Tangents.SetNumUninitialized(NumVerts);

    for (int32 i = 0; i < NumVerts; ++i)
    {
        Verts[i] = FVector(Mesh.Positions[i]);
        //Normals[i] = Mesh.Normals.IsValidIndex(i) ? FVector(Mesh.Normals[i]) : FVector(0, 0, 1);
        //UVs[i] = Mesh.UVs.IsValidIndex(i) ? FVector2D(Mesh.UVs[i]) : FVector2D::ZeroVector;
        //FColor C = Mesh.Colors.IsValidIndex(i) ? Mesh.Colors[i] : FColor::White;
        //Colors[i] = FLinearColor(C);
        //Tangents[i] = FProcMeshTangent(1, 0, 0);
    }

    Tris.SetNumUninitialized(Mesh.Indices.Num());
    for (int32 i = 0; i < Mesh.Indices.Num(); ++i)
        Tris[i] = (int32)Mesh.Indices[i];

    UProceduralMeshComponent* PMC = nullptr;
    if (TObjectPtr<UProceduralMeshComponent>* Existing = ChunkCollisionProxies.Find(Key))
    {
        if (IsValid(*Existing)) PMC = *Existing;
    }

    if (!PMC)
    {
        PMC = NewObject<UProceduralMeshComponent>(this, NAME_None, RF_Transient);
        if (!PMC) return;

        PMC->SetupAttachment(this);
        PMC->bUseAsyncCooking = true;
        PMC->bUseComplexAsSimpleCollision = true;
        PMC->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        PMC->SetCollisionObjectType(ECC_WorldStatic);
        PMC->SetCollisionResponseToAllChannels(ECR_Block);
        PMC->SetGenerateOverlapEvents(false);
        PMC->SetVisibility(false);
        PMC->SetHiddenInGame(true);

        PMC->RegisterComponent();
        ChunkCollisionProxies.Add(Key, PMC);
    }

    PMC->CreateMeshSection_LinearColor(0, Verts, Tris, Normals, UVs, Colors, Tangents, true, true);
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

    if (TouchedChunks.Num() == 0) return;

    {
        const double KeepUntil = FPlatformTime::Seconds() + CollisionKeepDuration;
        for (const FChunkKey& Key : TouchedChunks) ChunkCollisionKeepTime.Add(Key, KeepUntil);
    }

    RebuildTouchedChunks(TouchedChunks);
}

void UVoxelTerrainComponent::RebuildTouchedChunks(const TSet<FChunkKey>& TouchedChunks)
{
    for (const FChunkKey& Key : TouchedChunks)
    {
        // ⭐ 无论 ChunkMeshes 有没有这个 Chunk，都必须增加版本号。
        // 这样如果它正在后台加载，加载完会发现版本不对，自动接力重新生成最新的。
        PendingRebuilds.Add(Key);

        int32& Ver = ChunkVersions.FindOrAdd(Key);
        Ver++;

        // 如果这个 Chunk 已经有网格并且已经加载在内存中，立刻发起新的生成任务
        if (ChunkMeshes.Contains(Key) && !RequestedChunks.Contains(Key))
        {
            RequestedChunks.Add(Key);
            SpawnChunkAsyncTask(Key, Ver);
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

void UVoxelTerrainComponent::ClearAllSaveData()
{
    FlushAllPendingSaves();

    FScopeLock Lock(&DataLock);
    ChunkModifications.Empty();
    FileLoadedChunks.Empty();
    DirtyChunks.Empty();
    ChunkCollisionKeepTime.Empty();

    FString Dir = GetWorldSaveDir();
    IFileManager::Get().DeleteDirectory(*Dir, false, true);

    //UE_LOG(LogTemp, Warning, TEXT("VoxelTerrain: 存档已清除，目录: %s"), *Dir);

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
    if (TickHandle.IsValid()) FTSTicker::GetCoreTicker().RemoveTicker(TickHandle);
    TickHandle.Reset();

    if (CollisionTickHandle.IsValid()) FTSTicker::GetCoreTicker().RemoveTicker(CollisionTickHandle);
    CollisionTickHandle.Reset();

    ClearAllCollisions();

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

    FlushAllPendingSaves();

    Super::EndPlay(Reason);
}