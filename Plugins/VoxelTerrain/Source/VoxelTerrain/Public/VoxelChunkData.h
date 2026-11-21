#pragma once
#include "CoreMinimal.h"
#include "VoxelTypes.h"

struct FVoxelChunkData
{
    TArray<FVoxel> Voxels;
    TArray<float> SurfaceGrid;   // ⭐ N*N，每列(ix, iy)的地表Z
    FVoxelChunkData()
    {
        const int32 N = VoxelConfig::PaddedStride;               // 35
        Voxels.SetNumUninitialized(N * N * N);
        FMemory::Memset(Voxels.GetData(), 0, Voxels.Num() * sizeof(FVoxel));

        SurfaceGrid.SetNumUninitialized(N * N);
        //FMemory::Memset(SurfaceGrid.GetData(), 0, SurfaceGrid.Num() * sizeof(float));
        
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
    FORCEINLINE float& SurfaceAt(int32 ix, int32 iy)
    {
        return SurfaceGrid[ix * VoxelConfig::PaddedStride + iy];
    }
    FORCEINLINE float SurfaceAt(int32 ix, int32 iy) const
    {
        return SurfaceGrid[ix * VoxelConfig::PaddedStride + iy];
    }
    FORCEINLINE const FVoxel& At(int32 x, int32 y, int32 z) const { return Voxels[Index(x, y, z)]; }
};