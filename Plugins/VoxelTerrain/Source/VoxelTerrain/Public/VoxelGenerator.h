#pragma once
#include "CoreMinimal.h"
#include "VoxelChunkData.h"

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
        float CaveThreshold = 0.35f;
    };

    FSettings Settings;

    void GenerateChunk(int32 ChunkX, int32 ChunkY, int32 ChunkZ, int32 LOD,
        FVoxel* OutVoxels) const;

private:
    float Fbm(const FVector& P) const;
    float SampleDensity(const FVector& WorldPos) const;
};