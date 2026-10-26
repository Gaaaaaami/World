#pragma once
#include "CoreMinimal.h"
#include "VoxelTypes.h"

struct FVoxelChunkData
{
    TArray<FVoxel> Voxels;

    FVoxelChunkData()
    {
        const int32 N = VoxelConfig::PaddedStride;               // 35
        Voxels.SetNumUninitialized(N * N * N);
        FMemory::Memset(Voxels.GetData(), 0, Voxels.Num() * sizeof(FVoxel));
    }

    // x,y,z 范围 [-1, ChunkSize+1]，即 -1..33
    FORCEINLINE int32 Index(int32 x, int32 y, int32 z) const
    {
        const int32 N = VoxelConfig::PaddedStride;
        return ((x + VoxelConfig::PaddedOffset) * N
            + (y + VoxelConfig::PaddedOffset)) * N
            + (z + VoxelConfig::PaddedOffset);
    }
    FORCEINLINE FVoxel& At(int32 x, int32 y, int32 z) { return Voxels[Index(x, y, z)]; }
    FORCEINLINE const FVoxel& At(int32 x, int32 y, int32 z) const { return Voxels[Index(x, y, z)]; }
};