// Fill out your copyright notice in the Description page of Project Settings.haoha

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "RHITest.h"
#include "RHIDensityTools.h"
#include "CxGameMode.generated.h"

/**
 * 
 */
UCLASS()
class WORLD_API ACxGameMode : public AGameModeBase
{
	GENERATED_BODY()
public:
	virtual void StartPlay() override;

	UPROPERTY()
	TObjectPtr<URHITest> RHITest = nullptr;



};
