#pragma once
#include "CoreMinimal.h"
#include "VoxelTypes.h"

class FVoxelGenerator
{
public:
    struct FSettings
    {
        float BaseWorldZ = 1600.f;
        float HillHeight = 2000.f;
        float NoiseFreq = 0.0005f;
        int32 Octaves = 5;
        float Lacunarity = 2.0f;
        float Persistence = 0.5f;
        int32 Seed = 1337;
    };

    FSettings Settings;

    /** 用噪声填充一个 33³ 的 chunk，坐标是 chunk 的整数坐标 (X, Y, Z, LOD) */
    void GenerateChunk(int32 ChunkX, int32 ChunkY, int32 ChunkZ,
        FVoxel* OutVoxels) const;

private:
    float Fbm(const FVector& P) const;
    float SampleDensity(const FVector& WorldPos) const;
};