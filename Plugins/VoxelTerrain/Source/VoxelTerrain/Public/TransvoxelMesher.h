#pragma once
#include "CoreMinimal.h"
#include "VoxelChunkData.h"
#include "VoxelMeshData.h"

class VOXELTERRAIN_API FTransvoxelMesher
{
public:
    static void GenerateRegular(const FVoxelChunkData& Chunk,
        float VoxelSize,
        int32 LOD,
        FVoxelMeshData& Out);

    // FaceDirection:
    // 0 = +X, 1 = -X
    // 2 = +Y, 3 = -Y
    // 4 = +Z, 5 = -Z
    static void GenerateTransition(const FVoxelChunkData& Chunk,
        float VoxelSize,
        int32 LOD,
        int32 FaceDirection,
        FVoxelMeshData& Out);
};