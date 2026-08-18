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
	void InitlizeRender(FRHICommandListImmediate& RHICmdList);
	void RenderDensityNoise(FRHICommandListImmediate& RHICmdList, FVector3f InNoisePosition );
	void GetPixelBuffer(FRHICommandListImmediate& RHICmdList, TArray<uint8>& buffer, int32 &RowPitch, int32 &BufferHeight);
	void SaveToRawRGBA(void* CpuData, int32 RowPitch, int32 BufferHeight, int32 Width, int32 Height, const FString& FilePath);

public:
	FRHITextureDesc TextureDesc;
	FTextureRHIRef TextureRef;
	FRHITextureCreateDesc TextureCreateDesc;
	FBufferRHIRef IndexBuffer;
	FBufferRHIRef DrawArgBuffer;

	FGraphicsPipelineStateInitializer PSOInit;
	FBufferRHIRef VertexBuffer;

	TShaderRef<FGlobalShader> GlobalPS;
	TShaderRef<FGlobalShader> GlobalVS;

	int32 Size;
};
