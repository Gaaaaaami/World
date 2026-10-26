#pragma once
#include "CoreMinimal.h"
#include "Components/PrimitiveComponent.h"
#include "Containers/Ticker.h"
#include "HAL/ThreadSafeCounter.h"
#include "VoxelMeshData.h"
#include "VoxelTypes.h"
#include "VoxelTerrainComponent.generated.h"

class UMaterialInterface;
class UProceduralMeshComponent;
class FVoxelSceneProxy;

UCLASS(ClassGroup = Voxel, meta = (BlueprintSpawnableComponent))
class VOXELTERRAIN_API UVoxelTerrainComponent : public UPrimitiveComponent
{
    GENERATED_BODY()

public:
    UVoxelTerrainComponent();

    UPROPERTY(EditAnywhere, Category = "Voxel")
    TObjectPtr<UMaterialInterface> TerrainMaterial;

    UPROPERTY(EditAnywhere, Category = "Voxel")
    bool bGenerateCollision = true;

    // ⭐ 改为按 Chunk 数量加载，不再受 VoxelSize 影响
    UPROPERTY(EditAnywhere, Category = "Voxel|LOD", meta = (DisplayName = "LOD0 Radius XY (Chunks)"))
    int32 LOD0RadiusChunksXY = 12;

    UPROPERTY(EditAnywhere, Category = "Voxel|LOD", meta = (DisplayName = "LOD0 Radius Z (Chunks)"))
    int32 LOD0RadiusChunksZ = 2;

    // ⭐ 碰撞范围（单位：Chunk 数量）
    UPROPERTY(EditAnywhere, Category = "Voxel", meta = (DisplayName = "Collision Radius (Chunks)"))
    int32 CollisionRadiusChunks = 1;

    UPROPERTY(EditAnywhere, Category = "Voxel")
    float UnloadDelay = 5.0f;

    UPROPERTY(EditAnywhere, Category = "Voxel", meta = (DisplayName = "Collision Keep Duration (s)"))
    float CollisionKeepDuration = 60.0f;

    UFUNCTION(BlueprintCallable, Category = "Voxel")
    void ApplyEdit(const FVector& WorldPos, float Radius, bool bIsAdd);

    UFUNCTION(BlueprintCallable, Category = "Voxel")
    void ClearAllSaveData();

    virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
    virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;
    virtual void GetUsedMaterials(TArray<UMaterialInterface*>& OutMaterials, bool bGetDebugMaterials = false) const override;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;

private:
    FIntVector WorldToBaseChunk(const FVector& WorldPos) const;
    void StreamTick();
    void FlushPendingPushToSceneProxy();
    void OnChunkGenerated(FChunkKey Key, TSharedPtr<FVoxelMeshData, ESPMode::ThreadSafe> Mesh, int32 Version);

    void UpdateCollisions(const FVector& CamLoc, double Now);
    void ProcessCollisionQueue();
    void CreateCollisionForChunk(const FChunkKey& Key, const FVoxelMeshData& Mesh);
    void RemoveCollisionForChunk(const FChunkKey& Key);
    void ClearAllCollisions();

    void RequestChunkImmediate(const FChunkKey& Key);
    void RebuildTouchedChunks(const TSet<FChunkKey>& TouchedChunks);
    void SpawnChunkAsyncTask(const FChunkKey& Key, int32 CurrentVersion);

    // 存档系统
    FString GetWorldSaveDir() const;
    FString GetChunkFilePath(const FChunkKey& Key) const;
    void SaveChunkModificationFile(const FChunkKey& Key);
    void SaveChunkModificationFileAsync(const FChunkKey& Key);
    void FlushAllPendingSaves();
    void LoadChunkModificationFile(const FChunkKey& Key);
    FVoxelModificationMap GatherModificationsForChunk(const FChunkKey& Key);

    // 核心数据
    TSet<FChunkKey> RequestedChunks;
    TMap<FChunkKey, TSharedPtr<FVoxelMeshData, ESPMode::ThreadSafe>> ChunkMeshes;
    TMap<FChunkKey, double> ChunkLastActiveTime;
    TMap<FChunkKey, double> ChunkCollisionKeepTime;

    TMap<FChunkKey, FVoxelModificationMap> ChunkModifications;
    TSet<FChunkKey> FileLoadedChunks;
    TSet<FChunkKey> DirtyChunks;

    TSharedPtr<FThreadSafeCounter> PendingSaveCounter;

    double LastEditTime = 0.0;
    FCriticalSection DataLock;

    TSet<FChunkKey> PendingRebuilds;
    TMap<FChunkKey, int32> ChunkVersions;

    TArray<FChunkKey> PendingPushToSceneProxy;
    TMap<FChunkKey, TObjectPtr<UProceduralMeshComponent>> ChunkCollisionProxies;
    TArray<FChunkKey> PendingCollisionAdds;
    TArray<FChunkKey> PendingCollisionRemoves;

    FTSTicker::FDelegateHandle TickHandle;
    FTSTicker::FDelegateHandle CollisionTickHandle;

    FBox LocalBounds = FBox(ForceInit);
    bool bBoundsDirty = false;
};