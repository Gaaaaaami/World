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
    // ⭐ 统一物理尺寸：所有 LOD 都是 3200 的大小
    static constexpr int32 BaseChunkSize = 32;
    static constexpr float BaseVoxelSize = 100.f;
    static constexpr float ChunkWorldSize = BaseChunkSize * BaseVoxelSize; // 3200

    // 兼容旧代码
    static constexpr int32 ChunkSize = BaseChunkSize;
    static constexpr int32 ChunkStride = BaseChunkSize + 1;
    static constexpr float VoxelSize = BaseVoxelSize;

    // ⭐ 动态获取：LOD 越高，体素数越少，但世界大小永远是 3200
    FORCEINLINE int32 GetChunkSize(int32 LOD)
    {
        return FMath::Max(1, BaseChunkSize >> LOD);
    }

    FORCEINLINE int32 GetChunkStride(int32 LOD)
    {
        return GetChunkSize(LOD) + 1;
    }

    FORCEINLINE float GetVoxelSize(int32 LOD)
    {
        return BaseVoxelSize * (1 << LOD);
    }
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

using FVoxelModificationMap = TMap<FIntVector, FVoxelWeight>;