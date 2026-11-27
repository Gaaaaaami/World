// Copyright 2026 GlimmerWorks Studio. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "Components/PrimitiveComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Containers/Ticker.h"
#include "VoxelMeshData.h"
#include "VoxelChunkData.h"
#include "VoxelTypes.h"
#include "Engine/VolumeTexture.h" 
#include "Containers/CircularQueue.h"
#include "VoxelTerrainComponent.generated.h"

class UVoxelGenerator;
class UMaterialInterface;
class FVoxelSceneProxy;
class UTexture2D;
class UProceduralMeshComponent;

/**
 * One 2D noise layer used by the voxel generator.
 *
 * The texture is sampled on the CPU (see UVoxelTerrainComponent::LoadNoiseTextureData),
 * converted to a float array once at BeginPlay and then shared with the generator
 * through UVoxelGenerator::GlobalNoiseLayers. All chunks read the same cached data,
 * so changing this at runtime has no effect until the component is restarted.
 */
USTRUCT(BlueprintType)
struct FVoxelNoiseLayerInfo
{
    GENERATED_BODY()

    /** Source texture. Must be square and at least 4x4; G8 / BGRA8 / RGBA8 are supported. */
    UPROPERTY(EditAnywhere, Category = "Noise")
    TObjectPtr<UTexture2D> Texture = nullptr;

    /** How many times the texture is repeated over the noise domain. */
    UPROPERTY(EditAnywhere, Category = "Noise")
    float Frequency = 1.0f;

    /** Contribution of this layer to the final density value. */
    UPROPERTY(EditAnywhere, Category = "Noise")
    float Amplitude = 1.0f;

    /** UV offset applied before sampling, useful to de-correlate layers. */
    UPROPERTY(EditAnywhere, Category = "Noise")
    FVector2D Offset = FVector2D::ZeroVector;
};

/**
 * One 3D (volume) noise layer used by the voxel generator.
 *
 * Same pipeline as FVoxelNoiseLayerInfo but sampled from a UVolumeTexture, so the
 * data is cubic (SizeX == SizeY == SizeZ) and feeds 3D density / cave shapes.
 */
USTRUCT(BlueprintType)
struct FVoxelNoiseLayer3DInfo
{
    GENERATED_BODY()

    /** Source volume texture. Must be cubic and at least 4^3. */
    UPROPERTY(EditAnywhere, Category = "Noise")
    TObjectPtr<UVolumeTexture> Texture = nullptr;

    /** How many times the volume is repeated over the noise domain. */
    UPROPERTY(EditAnywhere, Category = "Noise")
    float Frequency = 1.0f;

    /** Contribution of this layer to the final density value. */
    UPROPERTY(EditAnywhere, Category = "Noise")
    float Amplitude = 1.0f;

    /** XYZ offset applied before sampling. */
    UPROPERTY(EditAnywhere, Category = "Noise")
    FVector Offset = FVector::ZeroVector;
};

/**
 * Vegetation rule for one species.
 *
 * A vertex of a generated chunk becomes a candidate for this species when the biome
 * texture (BiomeTexture) at that XY position decodes to an ID inside [MinID, MaxID].
 * Candidates then pass a deterministic per-vertex density test, and finally a
 * radius-based overlap rejection so plants do not intersect each other.
 *
 * Several configs may share the same ID range to mix species in one biome.
 */
USTRUCT(BlueprintType)
struct FVoxelVegetationConfig
{
    GENERATED_BODY()

    // 这个模型在哪个噪点区间生成（支持多个模型填同样的区间来实现混合）
    UPROPERTY(EditAnywhere, Category = "Vegetation")
    float MinID = 0.f;

    UPROPERTY(EditAnywhere, Category = "Vegetation")
    float MaxID = 130.f;

    /** Mesh spawned for every accepted instance. */
    UPROPERTY(EditAnywhere, Category = "Vegetation")
    TObjectPtr<UStaticMesh> Mesh = nullptr;

    /** Probability (0..1) that a candidate vertex actually spawns an instance. */
    UPROPERTY(EditAnywhere, Category = "Vegetation")
    float Density = 0.05f;

    UPROPERTY(EditAnywhere, Category = "Vegetation")
    float PlaceRadius = 100.f; // 防重叠半径

    /** Random uniform scale range applied per instance. */
    UPROPERTY(EditAnywhere, Category = "Vegetation")
    float MinScale = 0.8f;

    UPROPERTY(EditAnywhere, Category = "Vegetation")
    float MaxScale = 1.2f;
};


/**
 * Voxel terrain component.
 *
 * Owns the whole runtime pipeline of a chunked, streamed voxel terrain:
 *
 *   1. Streaming      - StreamTick() (a ~0.16s core ticker) requests chunks around the
 *                       local pawn and evicts chunks that left the radius.
 *   2. Generation     - SpawnChunkAsyncTask() runs voxel generation + Transvoxel meshing
 *                       on the thread pool, then hands the result back on the game thread
 *                       via OnChunkGenerated().
 *   3. Rendering      - The result is forwarded to FVoxelSceneProxy on the render thread;
 *                       this component itself only keeps CPU-side copies for editing.
 *   4. Editing        - ApplyEdit() stamps a sphere brush into the cached voxel data of
 *                       the touched chunks and rebuilds them synchronously.
 *   5. Collision      - A pool of UProceduralMeshComponent is recycled by
 *                       ProcessCollisionQueue() (a ~0.33s core ticker) around the pawn.
 *   6. Vegetation     - Optional, driven by a biome texture + FVoxelVegetationConfig list,
 *                       instanced through a pool of UHierarchicalInstancedStaticMeshComponent.
 *
 * Threading contract: every TMap / TSet member below is owned by the game thread.
 * Background tasks only ever read data that was captured by value / shared pointer,
 * and always re-enter through AsyncTask(ENamedThreads::GameThread, ...).
 */
UCLASS(ClassGroup = Voxel, Blueprintable, meta = (BlueprintSpawnableComponent))
class VOXELTERRAIN_API UVoxelTerrainComponent : public UPrimitiveComponent
{
    GENERATED_BODY()

public:
    UVoxelTerrainComponent();

    /** Material used by FVoxelSceneProxy for every chunk. Falls back to the engine default surface material. */
    UPROPERTY(EditAnywhere, Category = "Voxel")
    TObjectPtr<UMaterialInterface> TerrainMaterial;

    /** Load radius along X/Y, expressed in LOD0 chunks (square footprint around the pawn). */
    UPROPERTY(EditAnywhere, Category = "Voxel|LOD", meta = (DisplayName = "LOD0 Radius XY (Chunks)"))
    int32 LOD0RadiusChunksXY = 8;

    /** Load radius along Z, expressed in LOD0 chunks (vertical slab around the pawn). */
    UPROPERTY(EditAnywhere, Category = "Voxel|LOD", meta = (DisplayName = "LOD0 Radius Z (Chunks)"))
    int32 LOD0RadiusChunksZ = 2;

    /** Grace period (seconds) a chunk must stay outside the unload radius before it is evicted. */
    UPROPERTY(EditAnywhere, Category = "Voxel")
    float UnloadDelay = 2.0f;

    /** Master switch for the ProceduralMesh collision pool. When off, ProcessCollisionQueue() releases everything. */
    UPROPERTY(EditAnywhere, Category = "Voxel")
    bool bGenerateCollision = true;

    /** Half-extent (in chunks) of the box kept collidable around the pawn. */
    UPROPERTY(EditAnywhere, Category = "Voxel|Collision")
    FIntVector CollisionRadius = FIntVector(1, 1, 1);

    /**
     * Stamps a sphere brush into the terrain.
     *
     * Writes an SDF-derived weight into the cached voxel data (ChunkVoxelData) of every
     * chunk overlapping the brush, including a one-voxel halo in the 26 neighbours, then
     * synchronously re-meshes the touched chunks, pushes them to the scene proxy and
     * refreshes their ProceduralMesh collision section.
     *
     * @param WorldPos  Brush centre in world space.
     * @param Radius    Brush radius in world units.
     * @param bIsAdd    true  -> add solid material (weight takes the min),
     *                  false -> carve material away (weight takes the max).
     *
     * Runs on the game thread. Chunks that are not loaded yet are skipped, so editing
     * far away from the streaming radius is a no-op.
     */
    UFUNCTION(BlueprintCallable, Category = "Voxel")
    void ApplyEdit(const FVector& WorldPos, float Radius, bool bIsAdd);

    virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
    virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;
    virtual void GetUsedMaterials(TArray<UMaterialInterface*>& OutMaterials, bool bGetDebugMaterials = false) const override;

protected:
    /** Creates the generator instance, loads noise / biome textures, prewarms both pools and installs the two core tickers. */
    virtual void BeginPlay() override;

    /** Removes both core tickers and destroys every pooled collision component. */
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;

public:
    // ==================== 核心逻辑 ====================

    /**
     * Converts a world position to the integer coordinate of the LOD0 chunk containing it.
     * Uses floor division, so negative coordinates map correctly.
     */
    FIntVector WorldToBaseChunk(const FVector& WorldPos) const;


    /**
     * Streaming heartbeat of the voxel terrain. Driven by a repeating core ticker (~0.16s),
     * NOT by per-frame tick. Each call:
     *   1. Flushes chunks that finished generating while the scene proxy was still null.
     *   2. Gathers every not-yet-requested chunk inside LOD0RadiusChunksXY / LOD0RadiusChunksZ
     *      around the local pawn, sorts them by distance, and dispatches up to
     *      MaxRequestsPerTick async generation tasks (budget scales with pawn speed).
     *   3. Evicts chunks that left the unload radius (load radius + 1) and have been idle
     *      longer than UnloadDelay, releasing their voxel cache, collision proxy and
     *      render-thread resources.
     *
     * Overridable in Blueprint: call Parent: StreamTick to keep the default load/unload
     * behaviour and hook your own logic (per-tick budget, debug visualisation)
     * around it.
     */
    UFUNCTION(BlueprintNativeEvent, Category = "Voxel|Streaming")
    void StreamTick();


    //void FlushPendingPushToSceneProxy();


    /**
     * Game-thread completion callback of SpawnChunkAsyncTask().
     *
     * Drops the result when Version no longer matches ChunkVersions (an edit happened
     * while the task was in flight, the synchronous rebuild wins), otherwise stores the
     * mesh in ChunkMeshes, the voxel cache in ChunkVoxelData, grows LocalBounds and
     * forwards the mesh to FVoxelSceneProxy on the render thread.
     *
     * @param Key      Chunk the payload belongs to.
     * @param Mesh     Meshed chunk produced on the thread pool (shared, thread-safe).
     * @param VoxelData Raw density data kept for later ApplyEdit() reads.
     * @param Version  Version the task was started with; used for staleness rejection.
     */
     // ⭐ 自然加载回调：4 参数（含 VoxelData）
    void OnChunkGenerated(
        FChunkKey Key,
        TSharedPtr<FVoxelMeshData, ESPMode::ThreadSafe> Mesh,
        TSharedPtr<FVoxelChunkData> VoxelData,
        int32 Version);

    // 自然加载：后台异步生成
    /**
     * Dispatches one chunk to the thread pool.
     *
     * Background work: UVoxelGenerator::GenerateChunk -> FTransvoxelMesher::GenerateRegular
     * -> per-vertex world offset + planar UVs + bounds. The resulting mesh and voxel data
     * are then marshalled back to the game thread and delivered to OnChunkGenerated().
     * Holds only weak pointers to this component and the generator, so an in-flight task
     * becomes a no-op if the component dies first.
     */
    void SpawnChunkAsyncTask(const FChunkKey& Key, int32 CurrentVersion);

    // ==================== 数据 ====================

    /** Meshed chunks currently resident, keyed by chunk key. Game thread only. */
    TMap<FChunkKey, TSharedPtr<FVoxelMeshData, ESPMode::ThreadSafe>> ChunkMeshes;

    /** CPU-side voxel (density/weight) cache, kept only so ApplyEdit() can read and rewrite it. */
    TMap<FChunkKey, TSharedPtr<FVoxelChunkData>> ChunkVoxelData;   // ⭐ 体素缓存

    /** Last time (FPlatformTime::Seconds) a chunk was loaded or edited; drives UnloadDelay. */
    TMap<FChunkKey, double> ChunkLastActiveTime;

    //TMap<FChunkKey, FVoxelModificationMap> ChunkModifications;

    /** Per-chunk version counter, bumped on every edit so stale async results are discarded. */
    TMap<FChunkKey, int32> ChunkVersions;

    /** Chunks with an async task in flight; prevents duplicate requests in StreamTick(). */
    TSet<FChunkKey> RequestedChunks;

    //TArray<FChunkKey> PendingPushToSceneProxy;

    /** Union of the bounds of all resident chunks, returned by CalcBounds(). */
    FBox LocalBounds = FBox(ForceInit);

    //bool bBoundsDirty = false;

    /** Handle of the ~0.16s core ticker that drives StreamTick(). */
    FTSTicker::FDelegateHandle TickHandle;

    /** Handle of the ~0.33s core ticker that drives ProcessCollisionQueue(). */
    FTSTicker::FDelegateHandle CollisionTickHandle;

    // ==================== 碰撞系统 ====================
public:
    /** Pre-allocates PoolSize hidden ProceduralMeshComponents and registers them (collision disabled until acquired). */
    void PrewarmCollisionPool(int32 PoolSize);

    /** Pops a component from the pool and enables QueryAndPhysics. Returns nullptr when the pool is exhausted. */
    UProceduralMeshComponent* AcquireCollisionComponent();

    /** Clears all mesh sections, disables collision and pushes the component back into the pool. */
    void ReturnCollisionComponent(UProceduralMeshComponent* PMC);

private:
    /**
     * Collision heartbeat, driven by a ~0.33s core ticker.
     *
     * Releases proxies that fell outside CollisionRadius, then creates proxies for every
     * resident chunk inside the radius. Editing does NOT go through here: ApplyEdit()
     * rebuilds the touched collision sections synchronously.
     */
    void ProcessCollisionQueue();

    /** Releases the collision proxy of one chunk back into the pool, if any. */
    void RemoveCollisionForChunk(const FChunkKey& Key);

    /** Destroys every active collision proxy and every pooled component. Called from EndPlay(). */
    void ClearAllCollisions();

    /** Free list of ProceduralMeshComponents. */
    UPROPERTY()
    TArray<TObjectPtr<UProceduralMeshComponent>> CollisionComponentPool;

    /** Chunks that currently own a collision component. */
    TMap<FChunkKey, TObjectPtr<UProceduralMeshComponent>> ChunkCollisionProxies;

public:
    /**
     * Reads every NoiseLayers / NoiseLayers3D texture into float arrays and publishes them
     * to the generator's global layer arrays.
     *
     * Two code paths: editor builds read FImage from the source texture, packaged builds
     * read the cooked platform mip data (PF_G8 / PF_B8G8R8A8 / PF_R8G8B8A8 only).
     * sRGB textures are linearised so both paths produce identical values.
     * Gated by the VOXEL_USE_NOISE_TEXTURE define.
     */
    void LoadNoiseTextureData();

    /** 2D noise layers, sampled once at BeginPlay. */
    UPROPERTY(EditAnywhere, Category = "Voxel|Noise")
    TArray<FVoxelNoiseLayerInfo> NoiseLayers;

    /** 3D volume noise layers, sampled once at BeginPlay. */
    UPROPERTY(EditAnywhere, Category = "Voxel|Noise3D")
    TArray<FVoxelNoiseLayer3DInfo> NoiseLayers3D;


private: // 植被系统

    /**
     * Reads BiomeTexture into CachedBiomeData as normalised R-channel floats.
     * Packaged path only: relies on the cooked platform mip data, so the texture must be
     * CPU-readable (no compression that hides the R channel). Square textures only.
     */
    void InitBiomeTexture();

    /**
     * Dispatches the per-chunk vegetation scatter to the thread pool.
     *
     * No-op when BiomeTexture, CachedBiomeData or VegetationConfigs is empty. The pure
     * calculation runs on the pool (CalculateVegetationForChunk) and the instances are
     * handed back to the game thread. NOTE: the game-thread side that pushes the instances
     * into HISM is currently empty, so nothing is spawned yet.
     */
    void DispatchVegetationTask(FChunkKey Key, TSharedPtr<FVoxelMeshData, ESPMode::ThreadSafe> Mesh);

    /**
     * Pure, side-effect-free scatter computation for one chunk.
     *
     * Steps: filter vertices by normal (upward facing only) -> bilinearly sample the biome
     * texture at the vertex XY -> decode a biome ID -> for every matching config run a
     * deterministic hash test against Density -> sort candidates by PlaceRadius -> reject
     * overlaps through a 200-unit spatial hash grid -> assign a random scale / yaw.
     *
     * @param Mesh        Chunk mesh to scatter over.
     * @param Key         Chunk key, used to seed the deterministic random generator.
     * @param LocalData   Flattened biome texture (R channel, 0..1).
     * @param LocalSize   Edge length of the biome texture.
     * @param WorldSize   World size the biome texture is stretched over.
     * @param Configs     Vegetation rules to evaluate.
     * @return Accepted instances (Location in world space, PlantId = config index).
     */
    TArray<FVegetationInstanceData> CalculateVegetationForChunk(
        const FVoxelMeshData& Mesh,
        const FChunkKey& Key,
        const TArray<float>& LocalData,
        int32 LocalSize,
        float WorldSize,
        const TArray<FVoxelVegetationConfig>& Configs) const;
public:
    /** Biome map: the R channel encodes the biome ID that selects which vegetation config applies. */
    UPROPERTY(EditAnywhere, Category = "Voxel|Vegetation")
    TObjectPtr<UTexture2D> BiomeTexture = nullptr;
private:
    /** CPU copy of BiomeTexture, filled by InitBiomeTexture(). */
    TArray<float> CachedBiomeData;
public:
    /** Vegetation rules; an empty array disables the whole vegetation system. */
    UPROPERTY(EditAnywhere, Category = "Voxel|Vegetation")
    TArray<FVoxelVegetationConfig> VegetationConfigs;
private:
    // ⭐ HISM 对象池
    /** Free list of instanced-mesh components, prewarmed in BeginPlay(). */
    UPROPERTY()
    TArray<TObjectPtr<UHierarchicalInstancedStaticMeshComponent>> HISMPool;

    // 记录每个 Chunk 当前正在使用的 HISM
    /** HISMs currently borrowed by each chunk, so they can be returned on unload. */
    TMap<FChunkKey, TArray<TObjectPtr<UHierarchicalInstancedStaticMeshComponent>>> ChunkActiveHISMs;

    // 核心方法
    /** Pre-allocates PoolSize hidden, collision-less HISMs. */
    void PrewarmHISMPool(int32 PoolSize);

    /**
     * Pops a HISM from the pool, clears it, assigns Mesh and makes it visible.
     * Falls back to creating a new component when the pool is empty, so it never fails.
     */
    UHierarchicalInstancedStaticMeshComponent* AcquireHISM(UStaticMesh* Mesh);

    /** Clears instances, hides the component and pushes it back into the pool. */
    void ReturnHISM(UHierarchicalInstancedStaticMeshComponent* HISM);

private:
    /** Generator instance, created from VoxelGeneratorClass in BeginPlay(). */
    UPROPERTY()
    TObjectPtr<UVoxelGenerator> VoxelGenerator = nullptr;

    /** Class used to spawn VoxelGenerator. Set this in the editor to swap generation strategies. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel", meta = (AllowPrivateAccess = "true"))
    TSubclassOf<UVoxelGenerator> VoxelGeneratorClass;
};