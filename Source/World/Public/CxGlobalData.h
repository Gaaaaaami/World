// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include <iostream>
#include <vector>
#include <tuple>
#include "CxGlobalData.generated.h"

/**
 * 
 */

UCLASS()
class WORLD_API UCxGlobalData : public UGameInstanceSubsystem
{
	GENERATED_BODY()
public:

	//struct FFloatArray
	//{
	//	std::vector<float> noise;
	//	int32 width;
	//	int32 height;
	//};


	virtual void Initialize(FSubsystemCollectionBase& Collection);

	/** Implement this for deinitialization of instances of the system */
	virtual void Deinitialize();
public:
	inline void AddNoise(UTexture2D &t);
	inline float GetNoise(int32 index, FVector Position);
	inline FVector2D PositionToUV(const FVector& Position, float WorldSize, bool bTile = true);

public:
};
