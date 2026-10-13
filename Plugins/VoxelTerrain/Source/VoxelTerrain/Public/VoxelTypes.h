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
    static constexpr int32 ChunkSize = 32;
    static constexpr int32 ChunkStride = ChunkSize + 1;   // 33
    static constexpr float VoxelSize = 100.f;           // cm£¨1 ÌåËØ = 1 Ã×£©
}

struct FChunkKey
{
    int32 X = 0, Y = 0, Z = 0;

    FChunkKey() = default;
    FChunkKey(int32 InX, int32 InY, int32 InZ) : X(InX), Y(InY), Z(InZ) {}

    bool operator==(const FChunkKey& O) const
    {
        return X == O.X && Y == O.Y && Z == O.Z;
    }
};

FORCEINLINE uint32 GetTypeHash(const FChunkKey& K)
{
    uint32 H = ::GetTypeHash(K.X);
    H = HashCombine(H, ::GetTypeHash(K.Y));
    H = HashCombine(H, ::GetTypeHash(K.Z));
    return H;
}