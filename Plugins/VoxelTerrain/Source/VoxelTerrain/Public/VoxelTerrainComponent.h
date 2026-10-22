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
    float LOD0RadiusXYWorld = 19200.f;

    UPROPERTY(EditAnywhere, Category = "Voxel")
    float LOD0RadiusZWorld = 9600.f;

    UPROPERTY(EditAnywhere, Category = "Voxel|LOD")
    TArray<float> LODRatios;

    UPROPERTY(EditAnywhere, Category = "Voxel")
    int32 CollisionRadius = 1;

    UPROPERTY(EditAnywhere, Category = "Voxel")
    float UnloadDelay = 5.0f;

    // 在途异步请求上限
    UPROPERTY(EditAnywhere, Category = "Voxel|Streaming")
    int32 MaxInFlightRequests = 48;

    // 每帧最多往渲染线程发多少个 chunk 的 GPU 命令
    // 值越大加载越快，值越小越平稳（掉帧越少）
    UPROPERTY(EditAnywhere, Category = "Voxel|Streaming")
    int32 MaxGPUCommandsPerFrame = 4;

    virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
    virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;
    virtual void GetUsedMaterials(TArray<UMaterialInterface*>& OutMaterials,
        bool bGetDebugMaterials = false) const override;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;

private:
    void GenerateOneChunk(const FChunkKey& Key);
    FVector ChunkOriginWorld(const FChunkKey& Key) const;

    int32 SelectLOD(const FVector& ChunkCenter, const FVector& CamLoc) const;
    void ComputeDesiredChunks(const FVector& CamLoc, TSet<FChunkKey>& Out) const;

    void StreamTick();
    void OnChunkGenerated(FChunkKey Key,
        TSharedPtr<FVoxelMeshData, ESPMode::ThreadSafe> Mesh);

    void ProcessLODSwitches();
    bool CollectReplacementFor(const FChunkKey& OldKey,
        TArray<TPair<FChunkKey, TSharedPtr<FVoxelMeshData, ESPMode::ThreadSafe>>>& OutNewChunks) const;
    void ExecuteAtomicSwitch(
        const TArray<FChunkKey>& OldKeys,
        const TArray<TPair<FChunkKey, TSharedPtr<FVoxelMeshData, ESPMode::ThreadSafe>>>& NewChunks);

    FIntVector WorldToBaseChunk(const FVector& WorldPos) const;

    // 分帧处理 GPU 命令
    void FlushPendingGPUCommands();

    // 待发送到渲染线程的 GPU 命令
    struct FPendingAdd
    {
        FChunkKey Key;
        TSharedPtr<FVoxelMeshData, ESPMode::ThreadSafe> Mesh;
    };
    TArray<FPendingAdd> PendingGPUAdds;
    TArray<FChunkKey>   PendingGPURemoves;

    TSet<FChunkKey> RequestedChunks;
    FTSTicker::FDelegateHandle TickHandle;
    FTSTicker::FDelegateHandle RenderTickHandle;

    TMap<FChunkKey, TSharedPtr<FVoxelMeshData, ESPMode::ThreadSafe>> ChunkMeshes;
    TMap<FChunkKey, TSharedPtr<FVoxelMeshData, ESPMode::ThreadSafe>> StagingMeshes;

    FBox LocalBounds = FBox(ForceInit);
    bool bBoundsDirty = false;

    TMap<FChunkKey, double> ChunkLastActiveTime;

    TSet<FChunkKey> CachedDesiredChunks;
    FVector LastDesiredCamLoc = FVector(MAX_flt, MAX_flt, MAX_flt);

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