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

    // ============================ 顶点颜色（可选，未使用）============================
    FColor ComputeColor(const FVector3f& WorldPos, const FVector3f& Normal)
    {
        constexpr float BiomeFreq = 0.001f;
        constexpr float BlendHalf = 0.5f;

        const FVector2D P2D((double)WorldPos.X, (double)WorldPos.Y);
        const float N = FMath::PerlinNoise2D(P2D * BiomeFreq);
        const float t = FMath::Clamp((N + BlendHalf) / (BlendHalf * 2.f), 0.f, 1.f);
        const float S = t * t * (3.f - 2.f * t);

        const FColor Forest(0, 200, 0);
        const FColor Desert(220, 180, 80);
        return FColor(
            (uint8)FMath::Lerp((float)Forest.R, (float)Desert.R, S),
            (uint8)FMath::Lerp((float)Forest.G, (float)Desert.G, S),
            (uint8)FMath::Lerp((float)Forest.B, (float)Desert.B, S),
            255);
    }
}

// ============================ GenerateRegular ============================
void FTransvoxelMesher::GenerateRegular(const FVoxelChunkData& Chunk,
    float VoxelSize,
    int32 LOD,
    const FVector3f& ChunkWorldOrigin,
    FVoxelMeshData& Out)
{
    Out.Reset();

    // ⭐ 预分配（避免 TArray 10+ 次扩容）
    Out.Positions.Reserve(4096);
    Out.Normals.Reserve(4096);
    Out.UVs.Reserve(4096);
    Out.SurfaceHeights.Reserve(4096);
    Out.Indices.Reserve(8192);

    const int32 ChunkN = VoxelConfig::ChunkSize;     // 32
    const int32 EdgeN = VoxelConfig::ChunkStride;   // 33（EdgeCache 用）
    const int32 PaddedN = VoxelConfig::PaddedStride;  // 35（VoxelsBase 用）
    const int32 PaddedNN = PaddedN * PaddedN;          // 1225
    const int32 S = 1 << LOD;
    const float WorldStep = VoxelSize * S;

    const float* SurfaceGridData = Chunk.SurfaceGrid.GetData();

    // ⭐ EdgeCache 静态复用（thread_local，只 malloc 一次）
    //    边起点坐标范围 0..ChunkN-1，所以用 EdgeN = 33 足够
    static thread_local TArray<int32> EdgeCache;
    const int32 Required = EdgeN * EdgeN * EdgeN * 3;
    if (EdgeCache.Num() < Required)
    {
        EdgeCache.SetNumUninitialized(Required);
    }
    FMemory::Memset(EdgeCache.GetData(), 0xFF, Required * sizeof(int32));
    int32* EdgeCacheData = EdgeCache.GetData();

    // ⭐ VoxelsBase 直指内存（偏移量用 PaddedNN / PaddedN）
    const FVoxel* VoxelsBase = Chunk.Voxels.GetData();

    for (int32 x = 0; x < ChunkN; ++x)
    {
        for (int32 y = 0; y < ChunkN; ++y)
        {
            for (int32 z = 0; z < ChunkN; ++z)
            {
                // ⭐ cell 基础索引只算 1 次（替代原来的 8 次 Chunk.At）
                //    Index(x, y, z) = (x+1)*PaddedNN + (y+1)*PaddedN + (z+1)
                const int32 BaseIdx = (x + 1) * PaddedNN + (y + 1) * PaddedN + (z + 1);

                // ⭐ 8 角点用偏移量直接访问（无乘法）
                int8 W[8];
                W[0] = VoxelsBase[BaseIdx].Weight;
                W[1] = VoxelsBase[BaseIdx + PaddedNN].Weight;
                W[2] = VoxelsBase[BaseIdx + PaddedN].Weight;
                W[3] = VoxelsBase[BaseIdx + PaddedNN + PaddedN].Weight;
                W[4] = VoxelsBase[BaseIdx + 1].Weight;
                W[5] = VoxelsBase[BaseIdx + PaddedNN + 1].Weight;
                W[6] = VoxelsBase[BaseIdx + PaddedN + 1].Weight;
                W[7] = VoxelsBase[BaseIdx + PaddedNN + PaddedN + 1].Weight;

                // ⭐ Case 用位运算（替代循环）
                const uint32 Case =
                    ((W[0] < 0) ? 0x01u : 0u) |
                    ((W[1] < 0) ? 0x02u : 0u) |
                    ((W[2] < 0) ? 0x04u : 0u) |
                    ((W[3] < 0) ? 0x08u : 0u) |
                    ((W[4] < 0) ? 0x10u : 0u) |
                    ((W[5] < 0) ? 0x20u : 0u) |
                    ((W[6] < 0) ? 0x40u : 0u) |
                    ((W[7] < 0) ? 0x80u : 0u);

                if (Case == 0 || Case == 255) continue;

                const uint8 Class = regularCellClass[Case];
                const RegularCellData& CD = regularCellData[Class];
                const unsigned short* VD = regularVertexData[Case];

                int32 VertexIndices[12];
                int32 VCount = 0;

                for (int v = 0; v < 12; ++v)
                {
                    const unsigned short Entry = VD[v];
                    if (Entry == 0) break;

                    const int32 A = (Entry >> 4) & 0x0F;
                    const int32 B = Entry & 0x0F;
                    if (A >= 8 || B >= 8) continue;

                    const FIntVector& OA = CornerOffsets[A];
                    const FIntVector& OB = CornerOffsets[B];

                    const int32 StartX = x + FMath::Min(OA.X, OB.X);
                    const int32 StartY = y + FMath::Min(OA.Y, OB.Y);
                    const int32 StartZ = z + FMath::Min(OA.Z, OB.Z);

                    int32 Dir;
                    if (OA.X != OB.X)      Dir = 0;
                    else if (OA.Y != OB.Y) Dir = 1;
                    else                   Dir = 2;

                    // ⭐ EdgeCache 用 EdgeN（边起点坐标范围 0..32）
                    const int32 EdgeIdxPos = ((StartX * EdgeN + StartY) * EdgeN + StartZ) * 3 + Dir;
                    int32& Cached = EdgeCacheData[EdgeIdxPos];

                    int32 GlobalIdx;
                    if (Cached >= 0)
                    {
                        GlobalIdx = Cached;
                    }
                    else
                    {
                        const float WA = (float)W[A];
                        const float WB = (float)W[B];
                        const float Den = WB - WA;
                        const float t = FMath::IsNearlyZero(Den) ? 0.5f : (-WA) / Den;

                        const float PAX = (float)(x + OA.X);
                        const float PAY = (float)(y + OA.Y);
                        const float PAZ = (float)(z + OA.Z);
                        const float PBX = (float)(x + OB.X);
                        const float PBY = (float)(y + OB.Y);
                        const float PBZ = (float)(z + OB.Z);

                        // ⭐ 手动 Lerp 展开
                        const FVector3f LocalP(
                            PAX + (PBX - PAX) * t,
                            PAY + (PBY - PAY) * t,
                            PAZ + (PBZ - PAZ) * t);
                        const FVector3f WorldP = LocalP * WorldStep;

                        // ⭐ ComputeGradient 内联（VoxelsBase 用 PaddedNN / PaddedN）
                        const int32 xi = FMath::RoundToInt(LocalP.X);
                        const int32 yi = FMath::RoundToInt(LocalP.Y);
                        const int32 zi = FMath::RoundToInt(LocalP.Z);
                        const int32 CenterIdx = (xi + 1) * PaddedNN + (yi + 1) * PaddedN + (zi + 1);

                        const float dx = (float)VoxelsBase[CenterIdx + PaddedNN].Weight
                            - (float)VoxelsBase[CenterIdx - PaddedNN].Weight;
                        const float dy = (float)VoxelsBase[CenterIdx + PaddedN].Weight
                            - (float)VoxelsBase[CenterIdx - PaddedN].Weight;
                        const float dz = (float)VoxelsBase[CenterIdx + 1].Weight
                            - (float)VoxelsBase[CenterIdx - 1].Weight;

                        // ⭐ 归一化（和原版 ComputeGradient 一致）
                        FVector3f Normal(dx, dy, dz);
                        if (Normal.SizeSquared() < 1e-6f)
                            Normal = FVector3f(0, 0, 1);
                        else
                            Normal = Normal.GetSafeNormal();

                        GlobalIdx = Out.Positions.Num();
                        const int32 pix = FMath::RoundToInt(LocalP.X) + VoxelConfig::PaddedOffset;
                        const int32 piy = FMath::RoundToInt(LocalP.Y) + VoxelConfig::PaddedOffset;
                        const float GroundZ = SurfaceGridData[pix * PaddedN + piy];

                        // ⭐ 优化 2：只算 Z，不构建 3D 向量
                        const float Depth = GroundZ - (WorldP.Z + ChunkWorldOrigin.Z);

                        Out.SurfaceHeights.Add(Depth);
                        Out.Positions.Add(WorldP);
                        Out.Normals.Add(Normal);
                        Out.UVs.Add(FVector2f::ZeroVector);

                        Cached = GlobalIdx;
                    }

                    VertexIndices[VCount] = GlobalIdx;
                    ++VCount;
                }

                const int32 TCount = CD.GetTriangleCount();
                for (int t = 0; t < TCount; ++t)
                {
                    const int32 I0 = CD.vertexIndex[t * 3 + 0];
                    const int32 I1 = CD.vertexIndex[t * 3 + 1];
                    const int32 I2 = CD.vertexIndex[t * 3 + 2];

                    if (I0 >= VCount || I1 >= VCount || I2 >= VCount) continue;

                    Out.Indices.Add(VertexIndices[I0]);
                    Out.Indices.Add(VertexIndices[I2]);
                    Out.Indices.Add(VertexIndices[I1]);
                }
            }
        }
    }
}

