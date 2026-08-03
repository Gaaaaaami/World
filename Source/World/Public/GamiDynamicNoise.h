// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "NoiseGenerator.h"
#include "GamiDynamicNoise.generated.h"

/**
 * 
 */
UCLASS()
class WORLD_API UGamiDynamicNoise : public UNoiseGenerator
{
	GENERATED_BODY()
public:
	UGamiDynamicNoise();
	virtual ~UGamiDynamicNoise();
public:
	//UFUNCTION(BlueprintCallable, Category = "Noise")
	virtual float SampleDensity(const FVector& WorldPosition) const;

};
