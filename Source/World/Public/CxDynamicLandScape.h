// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProceduralMeshComponent/Public/ProceduralMeshComponent.h"
#include "ProceduralMeshComponent/Public/KismetProceduralMeshLibrary.h"
#include "brutus.h"
#include "Transvoxel.h"
#include "CxDynamicLandScape.generated.h"

USTRUCT(BlueprintType)
struct FDensityInformation
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadWrite)
	TArray<float> Density;
	UPROPERTY(BlueprintReadWrite)
	TArray<FVector> Position;
};

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
	UFUNCTION(BlueprintCallable, Category = "Voxel")
	void CreateDensityField(int32 Padding);
	UFUNCTION(BlueprintCallable, Category = "Voxel")
	inline void SetDensityField(int32 x, int32 y, int32 z, int Weight);
	UFUNCTION(BlueprintCallable, Category = "Voxel") 
	void UpdateMeshFromDensityMC();
	UFUNCTION(BlueprintCallable, Category = "Voxel")
	FDensityInformation GetDensityX(int32 x);
	UFUNCTION(BlueprintCallable, Category = "Voxel")
	FDensityInformation GetDensity();
	UFUNCTION(BlueprintCallable, Category = "Voxel")
	FDensityInformation GetLODDensity(int32 InX, int32 InLodLevel, FString InLogName);
	UFUNCTION(BlueprintCallable, Category = "Voxel")
	inline void DebugWriteArray2Desk(FString InLogName);




public:
	/* ========== 属性 ========== */

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ProceduralMesh")
	UProceduralMeshComponent* ProduralMeshComponent = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel|Material")
	UMaterialInterface* VoxelMaterial = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel|Settings")
	FVector LandscapeDimension = FVector(4, 4, 4); // 体素分辨率
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel|Settings")
	float  VoxelSize = 100.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel|Settings")
	float  FadeStrength = 0.4f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel|Debug")
	TObjectPtr<UStaticMesh> RuntimeMeshAsset; // 这个就是你在 Edit 里能点开看的资产


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel|Debug")
	int32 DeleteVoxel = 1;

	TArray<float> m_DensityData;  // 密度场数据
private:
	/* ========== 数据缓存 ========== */
	bool          m_initialize = false;

	Brutus::Grid* BrutusGrid = nullptr;

};