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

    UPROPERTY(EditAnywhere, Category = "Voxel|LOD")
    int32 LOD0RadiusXY = 2;

    UPROPERTY(EditAnywhere, Category = "Voxel|LOD")
    int32 LOD0RadiusZ = 1;

    UPROPERTY(EditAnywhere, Category = "Voxel|LOD")
    int32 LOD1RadiusXY = 4;

    UPROPERTY(EditAnywhere, Category = "Voxel|LOD")
    int32 LOD1RadiusZ = 2;

    UPROPERTY(EditAnywhere, Category = "Voxel")
    int32 CollisionRadius = 1;

    UPROPERTY(EditAnywhere, Category = "Voxel")
    float UnloadDelay = 5.0f;

    UFUNCTION(BlueprintCallable, Category = "Voxel")
    void ApplyEdit(const FVector& WorldPos, float Radius, bool bIsAdd);

    UFUNCTION(BlueprintCallable, Category = "Voxel")
    void SaveModifications(const FString& FilePath);

    UFUNCTION(BlueprintCallable, Category = "Voxel")
    void LoadModifications(const FString& FilePath);

    virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
    virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;
    virtual void GetUsedMaterials(TArray<UMaterialInterface*>& OutMaterials,
        bool bGetDebugMaterials = false) const override;

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

    TSet<FChunkKey> RequestedChunks;
    TMap<FChunkKey, TSharedPtr<FVoxelMeshData, ESPMode::ThreadSafe>> ChunkMeshes;
    TMap<FChunkKey, double> ChunkLastActiveTime;
    FVoxelModificationMap TerrainModifications;

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