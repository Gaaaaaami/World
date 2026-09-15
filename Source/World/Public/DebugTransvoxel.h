// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "brutus.h"
#include "Transvoxel.h"
#include "DebugTransvoxel.generated.h"

UCLASS()
class WORLD_API ADebugTransvoxel : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ADebugTransvoxel();

public:
	/**
	 * 生成过渡网格（Transition Mesh），用于连接相邻LOD的体素块
	 * @param OutVerts 输出的顶点数组
	 * @param OutTris 输出的三角形索引数组（顺时针绕序，符合UE默认）
	 * @param InHighRes 高LOD块的SDF密度数组（<0为实体，≥0为空气，和Transvoxel约定一致）
	 * @param InHighPos 高LOD块的顶点位置数组（和InHighRes一一对应）
	 * @param HighSize 高LOD块的尺寸（含Padding，X/Y/Z分量分别为X/Y/Z方向的格点数）
	 * @param Stride 高LOD的格点步长，等于2^LOD层级（例如LOD0为1，LOD1为2，以此类推）
	 * @param Face 过渡面标识，对应6个朝向：
	 *              0 = NegX（X负方向，块左侧面）
	 *              1 = PosX（X正方向，块右侧面）
	 *              2 = NegY（Y负方向，块前侧面）
	 *              3 = PosY（Y正方向，块后侧面）
	 *              4 = NegZ（Z负方向，块底面）
	 *              5 = PosZ（Z正方向，块顶面）
	 * @param Margin 插值t值的clamp边界，防止退化三角形，默认0.02f
	 */
	UFUNCTION(BlueprintCallable, Category = "Transvoxel")
	void BuildTransition_MultiUnit(
		TArray<FVector>& OutVerts,
		TArray<int32>& OutTris,
		const TArray<float>& InHighRes,
		const TArray<float>& InLowRes,
		const TArray<FVector>& InHighPos,
		const TArray<FVector>& InLowPos,
		FVector InHighResSize,
		FVector InLowResSize);
	
public:
	inline int32 CaculateLowLODDensityFieldIndex(int32 z, int32 y);

	UFUNCTION(BlueprintCallable, Category = "Voxel")
	void DebugWriteLowLODDensityField2Desk(int32 Width, int32 Height);

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel|Settings")
	TArray<float> LowLODDensityField;
	
	FVector LowLODDensitySize;
};
