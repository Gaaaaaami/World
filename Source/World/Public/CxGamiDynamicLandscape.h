// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PlanetActor.h"
#include "CxGamiDynamicLandscape.generated.h"

UCLASS()
class WORLD_API ACxGamiDynamicLandscape : public APlanetActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ACxGamiDynamicLandscape();
	virtual ~ACxGamiDynamicLandscape();
public:
	void GetDistanceFieldFromStaticMesh(UStaticMesh *InSourceMesh, TArray<float> &InDensity, int32 &InX, int32 &InY, int32 &InZ);
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxelizer")
	UStaticMesh* SourceMesh;

};
