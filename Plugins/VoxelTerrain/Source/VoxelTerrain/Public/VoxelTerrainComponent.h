#pragma once
#include "CoreMinimal.h"
#include "Components/PrimitiveComponent.h"
#include "Containers/Ticker.h"
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

    // ⭐ LOD0 范围（单位：厘米 CM）
    UPROPERTY(EditAnywhere, Category = "Voxel|LOD", meta = (DisplayName = "LOD0 Radius XY (cm)"))
    float LOD0RadiusXY = 10000.f;

    UPROPERTY(EditAnywhere, Category = "Voxel|LOD", meta = (DisplayName = "LOD0 Radius Z (cm)"))
    float LOD0RadiusZ = 5000.f;

    // ⭐ LOD1 范围（单位：厘米 CM）
    UPROPERTY(EditAnywhere, Category = "Voxel|LOD", meta = (DisplayName = "LOD1 Radius XY (cm)"))
    float LOD1RadiusXY = 25000.f;

    UPROPERTY(EditAnywhere, Category = "Voxel|LOD", meta = (DisplayName = "LOD1 Radius Z (cm)"))
    float LOD1RadiusZ = 15000.f;

    // 碰撞范围（单位：厘米 CM）
    UPROPERTY(EditAnywhere, Category = "Voxel", meta = (DisplayName = "Collision Radius (cm)"))
    float CollisionRadius = 4000.f;

    UPROPERTY(EditAnywhere, Category = "Voxel")
    float UnloadDelay = 5.0f;

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

    void UpdateCollisions(const FVector& CamLoc);
    void ProcessCollisionQueue();
    void CreateCollisionForChunk(const FChunkKey& Key, const FVoxelMeshData& Mesh);
    void RemoveCollisionForChunk(const FChunkKey& Key);
    void ClearAllCollisions();

    void RebuildChunksAround(const FVector& WorldPos, float Radius);
    void RequestChunkImmediate(const FChunkKey& Key);

    // ====== 存档系统 ======
    FString GetWorldSaveDir() const;
    FString GetChunkFilePath(const FChunkKey& Key) const;
    void SaveChunkModificationFile(const FChunkKey& Key);
    void LoadChunkModificationFile(const FChunkKey& Key);
    FVoxelModificationMap GatherModificationsForChunk(const FChunkKey& Key);

    // 核心数据
    TSet<FChunkKey> RequestedChunks;
    TMap<FChunkKey, TSharedPtr<FVoxelMeshData, ESPMode::ThreadSafe>> ChunkMeshes;
    TMap<FChunkKey, double> ChunkLastActiveTime;

    TMap<FChunkKey, FVoxelModificationMap> ChunkModifications;
    TSet<FChunkKey> FileLoadedChunks;
    TSet<FChunkKey> DirtyChunks;

    // ⭐ 防抖与线程锁
    double LastEditTime = 0.0;
    FCriticalSection DataLock; // 保护 ChunkModifications 多线程安全

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