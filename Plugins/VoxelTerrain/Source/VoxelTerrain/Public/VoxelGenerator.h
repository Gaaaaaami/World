#pragma once
#include "CoreMinimal.h"
#include "VoxelChunkData.h"
#include "VoxelTypes.h"
#ifndef VOXEL_USE_NOISE_TEXTURE
#define VOXEL_USE_NOISE_TEXTURE 1   // 1 = 开启贴图，0 = 退回原版
#endif
class VOXELTERRAIN_API FVoxelGenerator
{
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

    struct FSettings
    {
        float BaseWorldZ = 1600.f;
        float HillHeight = 2000.f;
        float NoiseFreq = 0.00035f;
        int32 Octaves = 5;
        float Lacunarity = 2.0f;
        float Persistence = 0.5f;
        int32 Seed = 1337;

        float CaveStrength = 600.f;
        float CaveThreshold = 0.15f;
    };

    FSettings Settings;

    void GenerateChunk(int32 ChunkX, int32 ChunkY, int32 ChunkZ, int32 LOD,
        const FVoxelModificationMap& Modifications,
        FVoxel* OutVoxels) const;

    // ⭐ 外部接口不变（ApplyEdit 用）
    float SampleDensity(const FVector& WorldPos) const;

private:
    // ⭐ 新增：把 SurfaceZ 拆出来，让 GenerateChunk 可以按 (X, Y) 列缓存 2D 噪声
    float SampleDensityWithSurface(const FVector& WorldPos, float SurfaceZ) const;

    float Fbm(const FVector& P) const;
    float Fbm2D(const FVector2D& P) const;
    float TexFbm2D(const FVector2D& WorldXY) const;
    float TexFbm3D(const FVector& WorldXYZ) const;
};