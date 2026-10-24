#pragma once
#include "CoreMinimal.h"
#include "VoxelTypes.h"

struct FVoxelChunkData
{
    TArray<FVoxel> Voxels;
    int32 CurrentLOD;

    FVoxelChunkData(int32 InLOD = 0) : CurrentLOD(InLOD)
    {
        const int32 N = VoxelConfig::GetChunkStride(InLOD);
        Voxels.SetNumUninitialized(N * N * N);
        FMemory::Memset(Voxels.GetData(), 0, Voxels.Num() * sizeof(FVoxel));
    }

    FORCEINLINE int32 Index(int32 x, int32 y, int32 z) const
    {
        const int32 N = VoxelConfig::GetChunkStride(CurrentLOD);
        return (x * N + y) * N + z;
    }
    FORCEINLINE FVoxel& At(int32 x, int32 y, int32 z) { return Voxels[Index(x, y, z)]; }
    FORCEINLINE const FVoxel& At(int32 x, int32 y, int32 z) const { return Voxels[Index(x, y, z)]; }
};