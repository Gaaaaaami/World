// Copyright 2026 GlimmerWorks Studio. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "VoxelChunkData.h"
#include "VoxelTypes.h"
#include "VoxelGenerator.generated.h"
#ifndef VOXEL_USE_NOISE_TEXTURE
#define VOXEL_USE_NOISE_TEXTURE 1   // 1 = 开启贴图，0 = 退回原版
#endif


USTRUCT(BlueprintType)

struct VOXELTERRAIN_API  FVoxelGeneratorSettings
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere,BlueprintReadWrite, Category = "Voxel")
    float BaseWorldZ = 1600.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel")
    float HillHeight = 2000.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel")
    float NoiseFreq = 0.00035f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel")
    int32 Octaves = 5;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel")
    float Lacunarity = 2.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel")
    float Persistence = 0.5f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel")
    int32 Seed = 1337;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel")
    float CaveStrength = 600.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel")
    float CaveThreshold = 0.15f;
};

UCLASS()
class VOXELTERRAIN_API UVoxelGenerator:public UObject
{
    GENERATED_BODY()

public:
    struct FNoiseLayerData
    {
        TSharedPtr<TArray<float>> Data;
        int32 Size = 0;
        float Frequency = 1.0f;
        float Amplitude = 1.0f;
        FVector2D Offset = FVector2D::ZeroVector;
    };
    struct FNoiseLayer3DData
    {
        TSharedPtr<TArray<float>> Data;
        int32 Size = 0;
        float Frequency = 1.0f;
        float Amplitude = 1.0f;
        FVector Offset = FVector::ZeroVector;
    };

    static TSharedPtr<TArray<FNoiseLayerData>> GlobalNoiseLayers;
    static TSharedPtr<TArray<FNoiseLayer3DData>> GlobalNoiseLayers3D;


    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel")
    FVoxelGeneratorSettings Settings;

    void GenerateChunk(int32 ChunkX,
                       int32 ChunkY, 
                       int32 ChunkZ, 
                       int32 LOD,
                       FVoxelChunkData& OutChunk) const;
public:
    
    virtual float SampleDensityWithSurface(const FVector& WorldPos, float SurfaceZ) const;


    virtual float Fbm(const FVector& P) const;
    virtual float Fbm2D(const FVector2D& P) const;
    virtual float TexFbm2D(const FVector2D& WorldXY) const;
    virtual float TexFbm3D(const FVector& WorldXYZ) const;
};
