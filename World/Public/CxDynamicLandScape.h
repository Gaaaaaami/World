// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProceduralMeshComponent/Public/ProceduralMeshComponent.h"
#include "ProceduralMeshComponent/Public/KismetProceduralMeshLibrary.h"
#include "CxDynamicLandScape.generated.h"

UCLASS()
class WORLD_API ACxDynamicLandScape : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ACxDynamicLandScape();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

public:
	/* ========== 体素核心功能 ========== */

	// 在指定的世界坐标处挖洞（Blueprint 也可调用）
	UFUNCTION(BlueprintCallable, Category = "Voxel")
	void DigHoleAtWorldLocation(FVector WorldLocation, float BrushRadiusCm = 150.f);
	UFUNCTION(BlueprintCallable, Category = "Voxel")
	void DigHoleAtWorldUnitLocation(FVector WorldLocation);
	// （可选）重新生成初始密度场
	UFUNCTION(BlueprintCallable, Category = "Voxel")
	void ResetLandscape();

	UFUNCTION(BlueprintCallable, Category = "Voxel")
	void DigHoleAtWorldLocation_Directed(
		FVector WorldLocation,
		float BrushRadiusCm,
		float Strength /*0~1*/,
		FVector Direction);
	// 根据当前密度场提取等值面并更新 ProceduralMesh
	void UpdateMeshFromDensity();
	/* ========== 内部方法 ========== */

// 生成/重置 Perlin 噪声密度场
	void GenerateDensityField(FVector dimSize);


public:
	/* ========== 属性 ========== */

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ProceduralMesh")
	UProceduralMeshComponent* m_procedural_mesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel|Material")
	UMaterialInterface* VoxelMaterial = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel|Settings")
	FVector LandscapeDimension = FVector(10, 10, 64); // 体素分辨率
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel|Settings")
	float  VoxelSize = 100.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel|Settings")
	float  NoiseHeight = 40.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel|Settings")
	float  FadeStrength = 0.4f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel|Settings")
	bool  test = false;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel|Debug")
	TObjectPtr<UStaticMesh> RuntimeMeshAsset; // 这个就是你在 Edit 里能点开看的资产

	TArray<float> m_DensityData;  // 密度场数据
private:
	/* ========== 数据缓存 ========== */
	bool          m_initialize = false;
};