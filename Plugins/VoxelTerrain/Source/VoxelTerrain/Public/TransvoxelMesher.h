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
};