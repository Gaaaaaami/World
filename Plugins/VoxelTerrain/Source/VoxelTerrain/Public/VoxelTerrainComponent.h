#pragma once
#include "CoreMinimal.h"
#include "Components/PrimitiveComponent.h"
#include "Containers/Ticker.h"
#include "VoxelMeshData.h"
#include "VoxelTypes.h"
#include "Engine/VolumeTexture.h" 
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
    float UnloadDelay = 5.0f;

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
    void OnChunkGenerated(FChunkKey Key, TSharedPtr<FVoxelMeshData, ESPMode::ThreadSafe> Mesh, int32 Version);
    void RebuildTouchedChunks(const TSet<FChunkKey>& TouchedChunks);
    void SpawnChunkAsyncTask(const FChunkKey& Key, int32 CurrentVersion);

    // ==================== 视觉网格数据 ====================
    TMap<FChunkKey, TSharedPtr<FVoxelMeshData, ESPMode::ThreadSafe>> ChunkMeshes;
    TMap<FChunkKey, double> ChunkLastActiveTime;
    TMap<FChunkKey, FVoxelModificationMap> ChunkModifications;
    TMap<FChunkKey, int32> ChunkVersions;

    // ⭐ 正在生成中的 chunk（防止重复派发）
    TSet<FChunkKey> RequestedChunks;

    // 编辑触发的重建标记（OnChunkGenerated 里用于判断 bWasRebuild）
    TSet<FChunkKey> PendingRebuilds;

    // 待推送渲染线程（SceneProxy 未就绪时暂存）
    TArray<FChunkKey> PendingPushToSceneProxy;

    FBox LocalBounds = FBox(ForceInit);
    bool bBoundsDirty = false;

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

    TSet<FChunkKey> PendingCollisionUpdates;

public:
    void LoadNoiseTextureData();

    UPROPERTY(EditAnywhere, Category = "Voxel|Noise")
    TArray<FVoxelNoiseLayerInfo> NoiseLayers;

    UPROPERTY(EditAnywhere, Category = "Voxel|Noise3D")
    TArray<FVoxelNoiseLayer3DInfo> NoiseLayers3D;
};