#include "TransvoxelMesher.h"
#include "Transvoxel.h"

namespace
{
    // ============================ 角点定义 ============================
    // 与 Transvoxel regularVertexData 的低字节端点一致：
    // 0:(0,0,0) 1:(1,0,0) 2:(0,1,0) 3:(1,1,0)
    // 4:(0,0,1) 5:(1,0,1) 6:(0,1,1) 7:(1,1,1)
    const FIntVector CornerOffsets[8] = {
        {0,0,0}, {1,0,0}, {0,1,0}, {1,1,0},
        {0,0,1}, {1,0,1}, {0,1,1}, {1,1,1}
    };

    // ============================ 采样权重 ============================
    FORCEINLINE float SampleWeight(const FVoxelChunkData& Chunk,
        int32 x, int32 y, int32 z)
    {
        const int32 N = VoxelConfig::ChunkStride;
        x = FMath::Clamp(x, 0, N - 1);
        y = FMath::Clamp(y, 0, N - 1);
        z = FMath::Clamp(z, 0, N - 1);
        return (float)Chunk.At(x, y, z).Weight;
    }

    // ============================ 梯度法线 ============================
    FVector3f ComputeGradient(const FVoxelChunkData& Chunk,
        const FVector3f& LocalPos)
    {
        const int32 xi = FMath::RoundToInt(LocalPos.X);
        const int32 yi = FMath::RoundToInt(LocalPos.Y);
        const int32 zi = FMath::RoundToInt(LocalPos.Z);

        const float dx = SampleWeight(Chunk, xi + 1, yi, zi)
            - SampleWeight(Chunk, xi - 1, yi, zi);
        const float dy = SampleWeight(Chunk, xi, yi + 1, zi)
            - SampleWeight(Chunk, xi, yi - 1, zi);
        const float dz = SampleWeight(Chunk, xi, yi, zi + 1)
            - SampleWeight(Chunk, xi, yi, zi - 1);

        FVector3f N(dx, dy, dz);
        if (N.SizeSquared() < 1e-6f)
            return FVector3f(0, 0, 1);
        return N.GetSafeNormal();
    }

    // ============================ 顶点颜色 ============================
    FColor ComputeColor(const FVector3f& WorldPos, const FVector3f& Normal)
    {
        const float Slope = 1.0f - FMath::Clamp(Normal.Z, 0.f, 1.f);
        const float Height = FMath::Clamp(WorldPos.Z / 3300.f, 0.f, 1.f);

        if (Slope > 0.45f)
            return FColor(110, 85, 60);      // 岩石棕
        if (Height > 0.65f)
            return FColor(240, 240, 245);    // 雪白
        return FColor(65, 125, 60);          // 草地绿
    }
}

// ============================ GenerateRegular ============================
void FTransvoxelMesher::GenerateRegular(const FVoxelChunkData& Chunk,
    float VoxelSize,
    int32 LOD,
    FVoxelMeshData& Out)
{
    Out.Reset();

    const int32 ChunkN = VoxelConfig::ChunkSize;
    const int32 S = 1 << LOD;
    const float WorldStep = VoxelSize * S;

    for (int32 x = 0; x < ChunkN; ++x)
        for (int32 y = 0; y < ChunkN; ++y)
            for (int32 z = 0; z < ChunkN; ++z)
            {
                // 1) 采样 8 个角点
                int8 W[8];
                for (int i = 0; i < 8; ++i)
                {
                    const FIntVector& O = CornerOffsets[i];
                    W[i] = Chunk.At(x + O.X, y + O.Y, z + O.Z).Weight;
                }

                // 2) case
                uint32 Case = 0;
                for (int i = 0; i < 8; ++i)
                    if (W[i] < 0) Case |= (1u << i);

                if (Case == 0 || Case == 255) continue;

                const uint8 Class = regularCellClass[Case];
                const RegularCellData& CD = regularCellData[Class];

                const int32 BaseVertex = Out.Positions.Num();

                // 3) 生成顶点
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
                    const FVector3f WorldP = LocalP * WorldStep;
                    const FVector3f Normal = ComputeGradient(Chunk, LocalP);

                    Out.Positions.Add(WorldP);
                    Out.Normals.Add(Normal);
                    Out.Colors.Add(ComputeColor(WorldP, Normal));
                    ++VCount;
                }

                // 4) 生成三角形（已修正绕序）
                const int32 TCount = CD.GetTriangleCount();
                for (int t = 0; t < TCount; ++t)
                {
                    const int32 I0 = CD.vertexIndex[t * 3 + 0];
                    const int32 I1 = CD.vertexIndex[t * 3 + 1];
                    const int32 I2 = CD.vertexIndex[t * 3 + 2];

                    if (I0 >= VCount || I1 >= VCount || I2 >= VCount) continue;

                    Out.Indices.Add(BaseVertex + I0);
                    Out.Indices.Add(BaseVertex + I2);
                    Out.Indices.Add(BaseVertex + I1);
                }
            }
}

// ============================ GenerateTransition ============================
void FTransvoxelMesher::GenerateTransition(const FVoxelChunkData& Chunk,
    float VoxelSize,
    int32 LOD,
    int32 FaceDirection,
    FVoxelMeshData& Out)
{
    const int32 ChunkN = VoxelConfig::ChunkSize;
    const int32 S = 1 << LOD;
    const float WorldStep = VoxelSize * S;

    // 13 个角点在过渡面上的 (u, v) 偏移。
    // 前 9 个是低分辨率 3x3 角点，间距 2 个高分辨率单位；
    // 后 4 个是高分辨率侧单元格中心。
    static const float CornerUV[13][2] = {
        {0.f, 0.f}, {1.f, 0.f}, {2.f, 0.f},
        {0.f, 1.f}, {1.f, 1.f}, {2.f, 1.f},
        {0.f, 2.f}, {1.f, 2.f}, {2.f, 2.f},
        {0.5f, 0.5f}, {1.5f, 0.5f},
        {0.5f, 1.5f}, {1.5f, 1.5f}
    };

    auto UVToLocal = [&](float BaseU, float BaseV, float u, float v) -> FVector3f
        {
            const float U = BaseU + u;
            const float V = BaseV + v;
            switch (FaceDirection)
            {
            case 0: return FVector3f((float)ChunkN, U, V); // +X
            case 1: return FVector3f(0.f, U, V);           // -X
            case 2: return FVector3f(U, (float)ChunkN, V); // +Y
            case 3: return FVector3f(U, 0.f, V);           // -Y
            case 4: return FVector3f(U, V, (float)ChunkN); // +Z
            case 5: return FVector3f(U, V, 0.f);           // -Z
            default: return FVector3f::ZeroVector;
            }
        };

    // 每个过渡单元格覆盖 2x2 个高分辨率单位，也就是 1x1 个低分辨率单位。
    for (int32 BaseU = 0; BaseU < ChunkN; BaseU += 2)
    {
        for (int32 BaseV = 0; BaseV < ChunkN; BaseV += 2)
        {
            // 1) 采样 13 个角点
            int8 W[13];
            for (int i = 0; i < 13; ++i)
            {
                const FVector3f Local = UVToLocal(
                    (float)BaseU, (float)BaseV,
                    CornerUV[i][0], CornerUV[i][1]);

                const int32 xi = FMath::Clamp(FMath::RoundToInt(Local.X), 0, ChunkN);
                const int32 yi = FMath::Clamp(FMath::RoundToInt(Local.Y), 0, ChunkN);
                const int32 zi = FMath::Clamp(FMath::RoundToInt(Local.Z), 0, ChunkN);

                W[i] = Chunk.At(xi, yi, zi).Weight;
            }

            // 2) 用前 9 个角点构造 9 位 case
            uint32 Case = 0;
            for (int i = 0; i < 9; ++i)
                if (W[i] < 0) Case |= (1u << i);

            if (Case == 0 || Case == 511) continue;

            // 3) 查表
            uint8 Class = transitionCellClass[Case];
            const bool bFlipWinding = (Class & 0x80) != 0;
            Class &= 0x7F;

            const TransitionCellData& CD = transitionCellData[Class];
            const unsigned short* VD = transitionVertexData[Case];

            const int32 BaseVertex = Out.Positions.Num();
            int32 VCount = 0;

            // 4) 生成顶点
            for (int v = 0; v < 12; ++v)
            {
                const unsigned short Entry = VD[v];
                if (Entry == 0) break;

                const int32 A = (Entry >> 4) & 0x0F;
                const int32 B = Entry & 0x0F;
                if (A > 12 || B > 12) continue;

                const float WA = (float)W[A];
                const float WB = (float)W[B];
                const float Den = WB - WA;
                const float t = FMath::IsNearlyZero(Den) ? 0.5f : (-WA) / Den;

                const FVector3f PA = UVToLocal(
                    (float)BaseU, (float)BaseV,
                    CornerUV[A][0], CornerUV[A][1]);
                const FVector3f PB = UVToLocal(
                    (float)BaseU, (float)BaseV,
                    CornerUV[B][0], CornerUV[B][1]);

                const FVector3f LocalP = FMath::Lerp(PA, PB, t);
                const FVector3f WorldP = LocalP * WorldStep;

                const FVector3f Normal = ComputeGradient(Chunk, LocalP);

                Out.Positions.Add(WorldP);
                Out.Normals.Add(Normal);
                Out.UVs.Add(FVector2f::ZeroVector);   
                Out.Colors.Add(ComputeColor(WorldP, Normal));
                ++VCount;
            }

            // 5) 生成三角形
            const int32 TCount = CD.GetTriangleCount();
            for (int t = 0; t < TCount; ++t)
            {
                const int32 I0 = CD.vertexIndex[t * 3 + 0];
                const int32 I1 = CD.vertexIndex[t * 3 + 1];
                const int32 I2 = CD.vertexIndex[t * 3 + 2];

                if (I0 >= VCount || I1 >= VCount || I2 >= VCount) continue;

                if (bFlipWinding)
                {
                    Out.Indices.Add(BaseVertex + I0);
                    Out.Indices.Add(BaseVertex + I2);
                    Out.Indices.Add(BaseVertex + I1);
                }
                else
                {
                    Out.Indices.Add(BaseVertex + I0);
                    Out.Indices.Add(BaseVertex + I1);
                    Out.Indices.Add(BaseVertex + I2);
                }
            }
        }
    }
}