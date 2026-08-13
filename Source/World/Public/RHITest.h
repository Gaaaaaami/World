// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "RHITest.generated.h"

/**
 * 
 */
UCLASS()
class WORLD_API URHITest : public UObject
{
	GENERATED_BODY()
public:
	URHITest();
	virtual ~URHITest();
public:
	virtual void PostInitProperties();
public:
	// 普通 UFUNCTION，游戏线程调用，内部投递到渲染线程
	UFUNCTION(BlueprintCallable)
	void Draw();
	void RenderTest(FRHICommandListImmediate& RHICmdList);
	void SaveToRawRGBA(void* CpuData, int32 RowPitch, int32 Width, int32 Height, const FString& FilePath);

};
