// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "RHIDensityTools.generated.h"

/**
 * 
 */
UCLASS()
class WORLD_API URHIDensityTools : public UObject
{
	GENERATED_BODY()
public:
	URHIDensityTools();
	virtual ~URHIDensityTools();
public:
	void Init(int32 InSize);
	void Draw();
	void RenderTest2D(FRHICommandListImmediate& RHICmdList);
	void SaveToRawRGBA(void* CpuData, int32 RowPitch, int32 BufferHeight, int32 Width, int32 Height, const FString& FilePath);


public:
	FRHITextureDesc TextureDesc;
	FTextureRHIRef TextureRef;
	FRHITextureCreateDesc TextureCreateDesc;
public:
	int32 Size;
};
