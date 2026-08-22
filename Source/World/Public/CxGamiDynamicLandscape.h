// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PlanetActor.h"
#include "Engine/Texture2D.h"
#include "CxGamiDynamicLandscape.generated.h"

UCLASS()
class WORLD_API ACxGamiDynamicLandscape : public APlanetActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ACxGamiDynamicLandscape();
	virtual ~ACxGamiDynamicLandscape();
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
public:
	inline double Now();
public:
	double PrevMS = 0.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Noise2DTexture")
	UTexture2D* Noise2DTexture = nullptr;

};
