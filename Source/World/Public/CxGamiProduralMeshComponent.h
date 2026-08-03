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

	void BindPlanetChunk(FPlanetChunk *InNewChunk);
	void BindPlanetActor(APlanetActor* InPlanetActor);

	UFUNCTION(BlueprintCallable, Category = "Planet")
	void Destruction(FVector InLocation, float BrushRadiusCm = 16.f, bool UpdateChunkCell = false);

public:
	FPlanetChunk *NewChunk;
	APlanetActor* PlanetActor;
	FVector Location;
};
