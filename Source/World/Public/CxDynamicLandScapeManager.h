#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CxDynamicLandScape.h"
#include "CxDynamicLandScapeManager.generated.h"

UCLASS()
class WORLD_API ACxDynamicLandScapeManager : public AActor
{
	GENERATED_BODY()

public:
	ACxDynamicLandScapeManager();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

public:
	/* ========== 配置（建议和 CxDynamicLandScape 保持一致） ========== */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel|Settings")
	int32 GridRadius = 2; // 生成 (2*R+1) x (2*R+1) 的区块
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel|Settings")
	int32 ChunkDimX = 32;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel|Settings")
	int32 ChunkDimY = 32;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel|Settings")
	int32 ChunkDimZ = 64;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel|Settings")
	float GlobalVoxelSize = 100.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel|Material")
	UMaterialInterface* VoxelMaterialRef; // 统一材质

	/* ========== 对外笔刷接口 ========== */
	UFUNCTION(BlueprintCallable, Category = "Voxel")
	void GlobalDigAtLocation(FVector WorldLocation, float BrushRadiusCm, float Strength = 1.0f);

private:
	/* ========== 内部管理 ========== */
	TMap<FIntPoint, ACxDynamicLandScape*> ChunkGrid;

	// 根据世界坐标拿到属于哪个 Chunk 网格
	FIntPoint WorldToGridCoord(const FVector& WorldLoc) const;

	// 把世界坐标转换到某个 Chunk 的局部体素坐标
	FVector WorldToLocalVoxel(const ACxDynamicLandScape* Chunk, const FVector& WorldLoc) const;

	// 初始化生成所有块
	void SpawnAllChunks();
};