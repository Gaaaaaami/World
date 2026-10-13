#pragma once
#include "CoreMinimal.h"
#include "VoxelTypes.h"

struct FVoxelChunkData
{
    TArray<FVoxel> Voxels;

    FVoxelChunkData()
    {
        const int32 N = VoxelConfig::ChunkStride;
        Voxels.SetNumUninitialized(N * N * N);
        FMemory::Memset(Voxels.GetData(), 0, Voxels.Num() * sizeof(FVoxel));
    }

    FORCEINLINE int32 Index(int32 x, int32 y, int32 z) const
    {
        return (x * VoxelConfig::ChunkStride + y) * VoxelConfig::ChunkStride + z;
    }
    FORCEINLINE FVoxel& At(int32 x, int32 y, int32 z) { return Voxels[Index(x, y, z)]; }
    FORCEINLINE const FVoxel& At(int32 x, int32 y, int32 z) const { return Voxels[Index(x, y, z)]; }
};