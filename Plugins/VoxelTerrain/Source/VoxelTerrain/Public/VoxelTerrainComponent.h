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

class UMaterialInterface;
class FVoxelSceneProxy;
class UTexture2D;
class UProceduralMeshComponent;

USTRUCT(BlueprintType)
struct FVoxelNoiseLayerInfo
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, Category = "Noise")
    TObjectPtr<UTexture2D> Texture = nullptr;

    UPROPERTY(EditAnywhere, Category = "Noise")
    float Frequency = 1.0f;

    UPROPERTY(EditAnywhere, Category = "Noise")
    float Amplitude = 1.0f;

    UPROPERTY(EditAnywhere, Category = "Noise")
    FVector2D Offset = FVector2D::ZeroVector;
};

USTRUCT(BlueprintType)
struct FVoxelNoiseLayer3DInfo
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, Category = "Noise")
    TObjectPtr<UVolumeTexture> Texture = nullptr;

    UPROPERTY(EditAnywhere, Category = "Noise")
    float Frequency = 1.0f;

    UPROPERTY(EditAnywhere, Category = "Noise")
    float Amplitude = 1.0f;

    UPROPERTY(EditAnywhere, Category = "Noise")
    FVector Offset = FVector::ZeroVector;
};

USTRUCT(BlueprintType)
struct FVoxelVegetationConfig
{
    GENERATED_BODY()

    // 这个模型在哪个噪点区间生成（支持多个模型填同样的区间来实现混合）
    UPROPERTY(EditAnywhere, Category = "Vegetation")
    float MinID = 0.f;

    UPROPERTY(EditAnywhere, Category = "Vegetation")
    float MaxID = 130.f;

    UPROPERTY(EditAnywhere, Category = "Vegetation")
    TObjectPtr<UStaticMesh> Mesh = nullptr;

    UPROPERTY(EditAnywhere, Category = "Vegetation")
    float Density = 0.05f;

    UPROPERTY(EditAnywhere, Category = "Vegetation")
    float PlaceRadius = 100.f; // 防重叠半径

    UPROPERTY(EditAnywhere, Category = "Vegetation")
    float MinScale = 0.8f;

    UPROPERTY(EditAnywhere, Category = "Vegetation")
    float MaxScale = 1.2f;
};


UCLASS(ClassGroup = Voxel, meta = (BlueprintSpawnableComponent))
class VOXELTERRAIN_API UVoxelTerrainComponent : public UPrimitiveComponent
{
    GENERATED_BODY()

public:
    UVoxelTerrainComponent();

    UPROPERTY(EditAnywhere, Category = "Voxel")
    TObjectPtr<UMaterialInterface> TerrainMaterial;

    UPROPERTY(EditAnywhere, Category = "Voxel|LOD", meta = (DisplayName = "LOD0 Radius XY (Chunks)"))
    int32 LOD0RadiusChunksXY = 8;

    UPROPERTY(EditAnywhere, Category = "Voxel|LOD", meta = (DisplayName = "LOD0 Radius Z (Chunks)"))
    int32 LOD0RadiusChunksZ = 2;

    UPROPERTY(EditAnywhere, Category = "Voxel")
    float UnloadDelay = 2.0f;

    UPROPERTY(EditAnywhere, Category = "Voxel")
    bool bGenerateCollision = true;

    UPROPERTY(EditAnywhere, Category = "Voxel|Collision")
    FIntVector CollisionRadius = FIntVector(1, 1, 1);

    UFUNCTION(BlueprintCallable, Category = "Voxel")
    void ApplyEdit(const FVector& WorldPos, float Radius, bool bIsAdd);

    virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
    virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;
    virtual void GetUsedMaterials(TArray<UMaterialInterface*>& OutMaterials, bool bGetDebugMaterials = false) const override;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;

private:
    // ==================== 核心逻辑 ====================
    FIntVector WorldToBaseChunk(const FVector& WorldPos) const;
    void StreamTick();
    void FlushPendingPushToSceneProxy();

    // ⭐ 自然加载回调：4 参数（含 VoxelData）
    void OnChunkGenerated(
        FChunkKey Key,
        TSharedPtr<FVoxelMeshData, ESPMode::ThreadSafe> Mesh,
        TSharedPtr<FVoxelChunkData> VoxelData,
        int32 Version);

    // 自然加载：后台异步生成
    void SpawnChunkAsyncTask(const FChunkKey& Key, int32 CurrentVersion);

    // ==================== 数据 ====================
    TMap<FChunkKey, TSharedPtr<FVoxelMeshData, ESPMode::ThreadSafe>> ChunkMeshes;
    TMap<FChunkKey, TSharedPtr<FVoxelChunkData>> ChunkVoxelData;   // ⭐ 体素缓存
    TMap<FChunkKey, double> ChunkLastActiveTime;
    //TMap<FChunkKey, FVoxelModificationMap> ChunkModifications;
    TMap<FChunkKey, int32> ChunkVersions;
    TSet<FChunkKey> RequestedChunks;
    TArray<FChunkKey> PendingPushToSceneProxy;
    FBox LocalBounds = FBox(ForceInit);
    //bool bBoundsDirty = false;

    FTSTicker::FDelegateHandle TickHandle;
    FTSTicker::FDelegateHandle CollisionTickHandle;

    // ==================== 碰撞系统 ====================
public:
    void PrewarmCollisionPool(int32 PoolSize);
    UProceduralMeshComponent* AcquireCollisionComponent();
    void ReturnCollisionComponent(UProceduralMeshComponent* PMC);

private:
    void ProcessCollisionQueue();
    void RemoveCollisionForChunk(const FChunkKey& Key);
    void ClearAllCollisions();

    UPROPERTY()
    TArray<TObjectPtr<UProceduralMeshComponent>> CollisionComponentPool;

    TMap<FChunkKey, TObjectPtr<UProceduralMeshComponent>> ChunkCollisionProxies;

public:
    void LoadNoiseTextureData();

    UPROPERTY(EditAnywhere, Category = "Voxel|Noise")
    TArray<FVoxelNoiseLayerInfo> NoiseLayers;

    UPROPERTY(EditAnywhere, Category = "Voxel|Noise3D")
    TArray<FVoxelNoiseLayer3DInfo> NoiseLayers3D;


private: // 植被系统
    void InitBiomeTexture();
    void DispatchVegetationTask(FChunkKey Key, TSharedPtr<FVoxelMeshData, ESPMode::ThreadSafe> Mesh);
    TArray<FVegetationInstanceData> CalculateVegetationForChunk(
        const FVoxelMeshData& Mesh,
        const FChunkKey& Key,
        const TArray<float>& LocalData,
        int32 LocalSize,
        float WorldSize,
        const TArray<FVoxelVegetationConfig>& Configs) const;
public:
    UPROPERTY(EditAnywhere, Category = "Voxel|Vegetation")
    TObjectPtr<UTexture2D> BiomeTexture = nullptr;
private:
    TArray<float> CachedBiomeData;
public:
    UPROPERTY(EditAnywhere, Category = "Voxel|Vegetation")
    TArray<FVoxelVegetationConfig> VegetationConfigs;
private:
    // ⭐ HISM 对象池
    UPROPERTY()
    TArray<TObjectPtr<UHierarchicalInstancedStaticMeshComponent>> HISMPool;

    // 记录每个 Chunk 当前正在使用的 HISM
    TMap<FChunkKey, TArray<TObjectPtr<UHierarchicalInstancedStaticMeshComponent>>> ChunkActiveHISMs;

    // 核心方法
    void PrewarmHISMPool(int32 PoolSize);
    UHierarchicalInstancedStaticMeshComponent* AcquireHISM(UStaticMesh* Mesh);
    void ReturnHISM(UHierarchicalInstancedStaticMeshComponent* HISM);
};