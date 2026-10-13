#include "TransvoxelMesher.h"
#include "Transvoxel.h"

namespace
{
    // Transvoxel 的角点约定：
    // 0:(0,0,0) 1:(1,0,0) 2:(0,1,0) 3:(1,1,0)
    // 4:(0,0,1) 5:(1,0,1) 6:(0,1,1) 7:(1,1,1)
    const FIntVector CornerOffsets[8] = {
        {0,0,0}, {1,0,0}, {0,1,0}, {1,1,0},
        {0,0,1}, {1,0,1}, {0,1,1}, {1,1,1}
    };

    /** 采样体素权重（自动 clamp 到 chunk 内部） */
    FORCEINLINE float SampleWeight(const FVoxelChunkData& Chunk, int32 x, int32 y, int32 z)
    {
        const int32 N = VoxelConfig::ChunkStride;
        x = FMath::Clamp(x, 0, N - 1);
        y = FMath::Clamp(y, 0, N - 1);
        z = FMath::Clamp(z, 0, N - 1);
        return (float)Chunk.At(x, y, z).Weight;
    }

    /**
     * 用中心差分计算梯度法线。
     *
     * Weight 约定：>0 是空气，<0 是实心。
     * 梯度方向 = 从实心指向空气 = 外法线方向。
     */
    FVector3f ComputeGradient(const FVoxelChunkData& Chunk, const FVector3f& LocalPos)
    {
        const int32 xi = FMath::RoundToInt(LocalPos.X);
        const int32 yi = FMath::RoundToInt(LocalPos.Y);
        const int32 zi = FMath::RoundToInt(LocalPos.Z);

        const float dx = SampleWeight(Chunk, xi + 1, yi, zi) - SampleWeight(Chunk, xi - 1, yi, zi);
        const float dy = SampleWeight(Chunk, xi, yi + 1, zi) - SampleWeight(Chunk, xi, yi - 1, zi);
        const float dz = SampleWeight(Chunk, xi, yi, zi + 1) - SampleWeight(Chunk, xi, yi, zi - 1);

        FVector3f N(dx, dy, dz);
        if (N.SizeSquared() < 1e-6f)
            return FVector3f(0, 0, 1);
        return N.GetSafeNormal();
    }
}





/**
 * 根据位置和法线算顶点颜色。
 *   - 陡坡（法线接近水平）→ 岩石棕
 *   - 高海拔 → 雪白
 *   - 其他 → 草地绿
 */
FColor ComputeColor(const FVector3f& WorldPos, const FVector3f& Normal)
{
    // 陡峭度：法线 Z 分量越小越陡
    const float Slope = 1.0f - FMath::Clamp(Normal.Z, 0.f, 1.f);

    // 归一化高度（假设 chunk 高约 33 * 100cm）
    const float Height = FMath::Clamp(WorldPos.Z / 3300.f, 0.f, 1.f);

    if (Slope > 0.45f)
    {
        return FColor(110, 85, 60);   // 岩石棕
    }
    if (Height > 0.65f)
    {
        return FColor(240, 240, 245); // 雪白
    }
    return FColor(65, 125, 60);       // 草地绿
}
void FTransvoxelMesher::GenerateRegular(const FVoxelChunkData& Chunk,
    float VoxelSize,
    FVoxelMeshData& Out)
{
    Out.Reset();

    const int32 N = VoxelConfig::ChunkSize;

    for (int32 x = 0; x < N; ++x)
        for (int32 y = 0; y < N; ++y)
            for (int32 z = 0; z < N; ++z)
            {
                // 采样 8 个角点
                int8 W[8];
                for (int i = 0; i < 8; ++i)
                {
                    const FIntVector& O = CornerOffsets[i];
                    W[i] = Chunk.At(x + O.X, y + O.Y, z + O.Z).Weight;
                }

                // 计算 case
                uint32 Case = 0;
                for (int i = 0; i < 8; ++i)
                    if (W[i] < 0) Case |= (1u << i);

                if (Case == 0 || Case == 255) continue;

                const uint8 Class = regularCellClass[Case];
                const RegularCellData& CD = regularCellData[Class];

                const int32 BaseVertex = Out.Positions.Num();

                // 生成该 cell 的顶点
                const unsigned short* VD = regularVertexData[Case];
                int32 VCount = 0;
                for (int v = 0; v < 12; ++v)
                {
                    const unsigned short Entry = VD[v];
                    if (Entry == 0) break;

                    const int32 A = (Entry >> 4) & 0x0F;
                    const int32 B = Entry & 0x0F;
                    if (A >= 8 || B >= 8) continue;

                    const float WA = (float)W[A];
                    const float WB = (float)W[B];
                    const float Den = WB - WA;
                    const float t = FMath::IsNearlyZero(Den) ? 0.5f : (-WA) / Den;

                    const FIntVector& OA = CornerOffsets[A];
                    const FIntVector& OB = CornerOffsets[B];
                    const FVector3f PA((float)(x + OA.X), (float)(y + OA.Y), (float)(z + OA.Z));
                    const FVector3f PB((float)(x + OB.X), (float)(y + OB.Y), (float)(z + OB.Z));

                    const FVector3f LocalP = FMath::Lerp(PA, PB, t);
                    const FVector3f WorldP = LocalP * VoxelSize;
                    const FVector3f Normal = ComputeGradient(Chunk, LocalP);

                    Out.Positions.Add(WorldP);
                    Out.Normals.Add(Normal);
                    Out.Colors.Add(ComputeColor(WorldP, Normal));
                    ++VCount;
                }

                // 生成三角形（保持 CW，见下面的说明）
                const int32 TCount = CD.GetTriangleCount();
                for (int t = 0; t < TCount; ++t)
                {
                    const int32 I0 = CD.vertexIndex[t * 3 + 0];
                    const int32 I1 = CD.vertexIndex[t * 3 + 1];
                    const int32 I2 = CD.vertexIndex[t * 3 + 2];

                    if (I0 >= VCount || I1 >= VCount || I2 >= VCount) continue;

                    // Transvoxel 官方绕序为 CW，交换 I1/I2 得到 UE 需要的 CCW
                    Out.Indices.Add(BaseVertex + I0);
                    Out.Indices.Add(BaseVertex + I2);
                    Out.Indices.Add(BaseVertex + I1);
                }
            }
}


