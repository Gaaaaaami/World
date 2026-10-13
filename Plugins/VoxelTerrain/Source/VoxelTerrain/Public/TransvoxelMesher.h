#pragma once
#include "CoreMinimal.h"
#include "VoxelChunkData.h"
#include "VoxelMeshData.h"

class FTransvoxelMesher
{
public:
    /** 只处理 regular cells。后续再加 LOD 过渡。 */
    static void GenerateRegular(const FVoxelChunkData& Chunk,
        float VoxelSize,
        FVoxelMeshData& Out);
};