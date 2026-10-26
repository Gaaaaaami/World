#pragma once
#include "CoreMinimal.h"

using FVoxelWeight = int8;

struct FVoxel
{
    FVoxelWeight Weight = 0;
    uint8        Material = 0;

    FORCEINLINE bool IsSolid() const { return Weight < 0; }
};

namespace VoxelConfig
{
    static constexpr int32 ChunkSize = 16;
    static constexpr int32 ChunkStride = ChunkSize + 1;         // 33：chunk 内部采样点
    static constexpr int32 PaddedStride = ChunkStride + 2;       // 35：加 ±1 的 halo
    static constexpr int32 PaddedOffset = 1;                      // padded 数组的索引偏移
    static constexpr float VoxelSize = 100.f;
}

struct FChunkKey
{
    int32 X = 0, Y = 0, Z = 0;
    int32 LOD = 0;

    FChunkKey() = default;
    FChunkKey(int32 InX, int32 InY, int32 InZ, int32 InLOD = 0)
        : X(InX), Y(InY), Z(InZ), LOD(InLOD) {
    }

    bool operator==(const FChunkKey& O) const
    {
        return X == O.X && Y == O.Y && Z == O.Z && LOD == O.LOD;
    }
};

FORCEINLINE uint32 GetTypeHash(const FChunkKey& K)
{
    uint32 H = ::GetTypeHash(K.X);
    H = HashCombine(H, ::GetTypeHash(K.Y));
    H = HashCombine(H, ::GetTypeHash(K.Z));
    H = HashCombine(H, ::GetTypeHash(K.LOD));
    return H;
}

// 修改记录：坐标 -> 新的密度权重
using FVoxelModificationMap = TMap<FIntVector, FVoxelWeight>;