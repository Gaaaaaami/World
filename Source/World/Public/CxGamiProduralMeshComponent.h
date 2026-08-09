// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ProceduralMeshComponent.h"
#include "PlanetChunk.h"
#include "PlanetActor.h"
#include "CxGamiProduralMeshComponent.generated.h"

/**
 * 
 */

UCLASS()
class WORLD_API UCxGamiProduralMeshComponent : public UProceduralMeshComponent
{
	GENERATED_BODY()
public:
	UCxGamiProduralMeshComponent(const FObjectInitializer& ObjectInitializer);
	virtual ~UCxGamiProduralMeshComponent();
public:
public:
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
public:
	void BindPlanetChunk(FPlanetChunk *InNewChunk);
	void BindPlanetActor(APlanetActor* InPlanetActor);

	UFUNCTION(BlueprintCallable, Category = "Planet")
	void Destruction(FVector InLocation, float BrushRadiusCm = 16.f, float Strength = 3.f, bool UpdateChunkCell = false);
	inline bool UpdateChunk(FVector ChunkLocation, FVector InLocation, float BrushRadiusCm, float Strength, bool UpdateChunkCell);
public:

public:
	FPlanetChunk *NewChunk;
	APlanetActor* PlanetActor;
	FVector Location;
};
