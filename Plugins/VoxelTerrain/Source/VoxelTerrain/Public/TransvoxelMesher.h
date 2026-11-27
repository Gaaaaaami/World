// Copyright 2026 GlimmerWorks Studio. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "VoxelChunkData.h"      // ⭐ 必须有这行
#include "VoxelMeshData.h"
class VOXELTERRAIN_API FTransvoxelMesher
{
public:
    // 自然加载（全量）
    static void GenerateRegular(const FVoxelChunkData& Chunk,
        float VoxelSize,
        int32 LOD,
        const FVector3f& ChunkWorldOrigin,
        FVoxelMeshData& Out);

    // 编辑地形（局部重建）
    static void GenerateRegularPartial(
        const FVoxelChunkData& Chunk,
        float VoxelSize,
        const FVector3f& ChunkWorldOrigin,
        const FIntVector& CellMin,
        const FIntVector& CellMax,
        FVoxelMeshData& InOut);
};