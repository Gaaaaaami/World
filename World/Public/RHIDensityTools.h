// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Containers/SpscQueue.h"
#include <functional>
#include "RHIDensityTools.generated.h"

/**
 * 
 */
UCLASS()

class WORLD_API URHIDensityTools : public UObject
{
	GENERATED_BODY()

public:
	typedef struct {
		FVector3f Location;
		FVector ChunkCenter;
	} stRenderTargetCommand;
public:
	URHIDensityTools();
	virtual ~URHIDensityTools();
public:
	void Init(int32 InSize);
	void Run();
	void InitlizeRender(FRHICommandListImmediate& RHICmdList);
	void RenderDensityNoise(FRHICommandListImmediate& RHICmdList, FVector3f InNoisePosition );
	void GetPixelBuffer(FRHICommandListImmediate& RHICmdList, TArray<float>& buffer, int32 &RowPitch, int32 &BufferHeight);
	void SaveToRawRGBA(void* CpuData, int32 RowPitch, int32 BufferHeight, int32 Width, int32 Height, const FString& FilePath);
	void Float2Uint8(TArray<float>& src, TArray<uint8>& dst, int32 RowPitch, int32 BufferHeight, int32 Width);

public:

	void InsertCommand(stRenderTargetCommand command);
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
	float ChunkSize = 100.f;
	std::function<void(TArray<TArray<float>>, TArray<FVector3f>, TArray<FVector>)> ReadCallBackFunction = nullptr;

	TSpscQueue<stRenderTargetCommand> CommandQueue;

};
