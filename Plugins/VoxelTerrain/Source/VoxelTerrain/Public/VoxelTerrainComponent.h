#pragma once
#include "CoreMinimal.h"
#include "Components/PrimitiveComponent.h"
#include "Containers/Ticker.h"
#include "VoxelMeshData.h"
#include "VoxelTypes.h"
#include "VoxelTerrainComponent.generated.h"

class UMaterialInterface;
class UProceduralMeshComponent;

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

    UPROPERTY(EditAnywhere, Category = "Voxel")
    int32 ViewRadius = 4;

    UPROPERTY(EditAnywhere, Category = "Voxel")
    int32 UnloadRadius = 3;

    UPROPERTY(EditAnywhere, Category = "Voxel")
    int32 LOD0Radius = 2;

    UPROPERTY(EditAnywhere, Category = "Voxel")
    int32 LOD1Radius = 4;

    UPROPERTY(EditAnywhere, Category = "Voxel")
    float UnloadDelay = 5.0f;

    virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
    virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;
    virtual void GetUsedMaterials(TArray<UMaterialInterface*>& OutMaterials,
        bool bGetDebugMaterials = false) const override;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;

private:
    void GenerateOneChunk(const FChunkKey& Key);
    //void PushAllChunksToRenderThread();
    FVector ChunkOriginWorld(const FChunkKey& Key) const;

    void StreamTick();
    void OnChunkGenerated(FChunkKey Key,
        TSharedPtr<FVoxelMeshData, ESPMode::ThreadSafe> Mesh);
    FIntVector WorldToBaseChunk(const FVector& WorldPos) const;
    TSet<FChunkKey> RequestedChunks;
    FTSTicker::FDelegateHandle TickHandle;

    TMap<FChunkKey, TSharedPtr<FVoxelMeshData, ESPMode::ThreadSafe>> ChunkMeshes;



    FBox LocalBounds = FBox(ForceInit);
    bool bBoundsDirty = false;

    // 每个 chunk 的最后活跃时间（用于延迟卸载）
    TMap<FChunkKey, double> ChunkLastActiveTime;
public:
    UPROPERTY(EditAnywhere, Category = "Voxel")
    int32 CollisionRadius = 1;

private:
    void UpdateCollisions(const FVector& CamLoc);
    void ProcessCollisionQueue();
    void CreateCollisionForChunk(const FChunkKey& Key, const FVoxelMeshData& Mesh);
    void RemoveCollisionForChunk(const FChunkKey& Key);
    void ClearAllCollisions();

    TMap<FChunkKey, TObjectPtr<UProceduralMeshComponent>> ChunkCollisionProxies;
    TArray<FChunkKey> PendingCollisionAdds;
    TArray<FChunkKey> PendingCollisionRemoves;
    FTSTicker::FDelegateHandle CollisionTickHandle;

};