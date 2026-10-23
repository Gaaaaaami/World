#pragma once
#include "CoreMinimal.h"
#include "VoxelChunkData.h"
#include "VoxelTypes.h"

class VOXELTERRAIN_API FVoxelGenerator
{
public:
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

    // ⭐ 移到了 public，以便在组件里做布尔运算查询
    float SampleDensity(const FVector& WorldPos) const;

private:
    float Fbm(const FVector& P) const;
    float Fbm2D(const FVector2D& P) const;
};