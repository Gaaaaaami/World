// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CxGamiProduralMeshComponent.h"
#include "ProceduralMeshComponent/Public/ProceduralMeshComponent.h"
#include "ProceduralMeshComponent/Public/KismetProceduralMeshLibrary.h"
#include "brutus.h"
#include "Transvoxel.h"
#include <functional>
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

USTRUCT(BlueprintType)
struct FChunkManagerInformation
{
	GENERATED_BODY()
	Brutus::Grid* BrutusGrid = nullptr;
	UPROPERTY()

	TObjectPtr<UCxGamiProduralMeshComponent> MeshComponent;

	UPROPERTY()
	FVector LandscapeDimension;
	UPROPERTY()
	float VoxelSize;
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
	UFUNCTION(BlueprintCallable, Category = "Voxel")
	void SetPlayerLocation(FVector InLocation);
public:
	inline float GetNoise(float InX, float InY, float InZ, const Brutus::Size3D &InTotal,const FVector &InComponentLocation, float InVoxelSize, uint8 InLOD);
	inline float GetNoise(FVector InLocation, const Brutus::Size3D& InTotal, const FVector& InComponentLocation, float InVoxelSize, uint8 InLOD);

public:
	bool CircleChunk(FVector InKey);

public:
	FDensityInformation GetDensityX(int32 InX, const FChunkManagerInformation & InChunkManagerInformation);


public:
	void CreateTransitionMesh(const Brutus::Size3D InTotal,
		FVector InVector, 
		FVector InLocation,
		float InLength,
		float InVoxelSize,
		TArray<FVector> &OutVertices, 
		TArray<int32> &OutTriangle);

	void CreateDensityField(const FChunkManagerInformation &InChunkManagerInformation);
	void UpdateMeshFromDensityMC(const FChunkManagerInformation &InChunkManagerInformation);
	void UpdateLOD(FVector InKEY, float InDistSqared);
	void ChunkExec();

public:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ProceduralMesh")
	UProceduralMeshComponent* ProduralMeshComponent = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel|Material")
	UMaterialInterface* VoxelMaterial = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel|Settings")
	FVector LandscapeDimension = FVector(4, 4, 4); // ÌåËØ·Ö±æÂÊ
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel|Settings")
	float  VoxelSize = 100.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel|Settings")
	float  FadeStrength = 0.4f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel")
	float LoadedRange = 10000.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel|Debug")
	int32 DeleteVoxel = 1;
	UPROPERTY()
	TMap<FVector, FChunkManagerInformation>	ChunkManager;
	UPROPERTY()
	FVector PlayerLocation;
	UPROPERTY()
	float LoadedRangeSqared = 0.f;
	UPROPERTY()

	float NoiseHeight = 200.f;
	UPROPERTY()

	float BaseHeight = 200.f;
	UPROPERTY()

	float NoiseFreq = 0.002f;


};