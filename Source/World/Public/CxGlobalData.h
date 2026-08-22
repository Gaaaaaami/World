// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "CxGlobalData.generated.h"

/**
 * 
 */

USTRUCT(BlueprintType)
struct FFloatArray
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadWrite)
	TArray<float> noise;
};

UCLASS()
class WORLD_API UCxGlobalData : public UGameInstanceSubsystem
{
	GENERATED_BODY()
public:


	virtual void Initialize(FSubsystemCollectionBase& Collection);

	/** Implement this for deinitialization of instances of the system */
	virtual void Deinitialize();

	UPROPERTY(BlueprintReadWrite)
	TArray<FFloatArray> Noise;
};
