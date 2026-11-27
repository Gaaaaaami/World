// Copyright 2026 GlimmerWorks Studio. All Rights Reserved.

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
        constexpr float BiomeFreq = 0.000001f;

        // ---- 分界（按你的实际地形高度再微调）----
        constexpr float SoilGrassBoundary = 2000.f;  // 草/土 分界，往下拉了 500
        constexpr float RockSoilBoundary = 800.f;   // 土/岩 分界

        // ---- 窄过渡带 ----
        constexpr float HeightBlendHalf = 60.f;      // 高度过渡总宽 120cm
        constexpr float BiomeBlendHalf = 0.05f;     // 横向噪声阈值带

        const FColor Forest(0, 200, 0);
        const FColor Desert(220, 180, 80);
        const FColor Soil(125, 95, 60);
        const FColor Rock(115, 115, 125);

        const FVector2D P2D((double)WorldPos.X, (double)WorldPos.Y);
        const float N = FMath::PerlinNoise2D(P2D * BiomeFreq);
        const float z = WorldPos.Z;

        // ---- 横向：森林 <-> 沙地（窄过渡）----
        const float sBiome = FMath::SmoothStep(-BiomeBlendHalf, BiomeBlendHalf, N);
        const FVector3f surfaceCol = FMath::Lerp(
            FVector3f(Forest.R, Forest.G, Forest.B),
            FVector3f(Desert.R, Desert.G, Desert.B),
            sBiome);

        // ---- 纵向：岩石 -> 土 -> 地表（窄过渡）----
        const float wSoil = FMath::SmoothStep(RockSoilBoundary - HeightBlendHalf,
            RockSoilBoundary + HeightBlendHalf, z);
        const float wGrass = FMath::SmoothStep(SoilGrassBoundary - HeightBlendHalf,
            SoilGrassBoundary + HeightBlendHalf, z);

        FVector3f C(Rock.R, Rock.G, Rock.B);
        C = FMath::Lerp(C, FVector3f(Soil.R, Soil.G, Soil.B), wSoil);
        C = FMath::Lerp(C, surfaceCol, wGrass);

        return FColor(
            (uint8)FMath::Clamp(FMath::RoundToInt(C.X), 0, 255),
            (uint8)FMath::Clamp(FMath::RoundToInt(C.Y), 0, 255),
            (uint8)FMath::Clamp(FMath::RoundToInt(C.Z), 0, 255),
            255);
    }


    // 注意：增加了一个 SurfaceHeight 参数（当前XY的地表最高点）
#define COMPUTECOLOR_FADE
#ifdef COMPUTECOLOR_FADE 

    FColor ComputeColor(const FVector3f& WorldPos, const FVector3f& Normal, float SurfaceHeight)
    {
        // 主频：特征尺度约 2km（上一版 500m 的 4 倍）
        constexpr float BiomeFreq = 0.000005f;

        float Depth = SurfaceHeight - WorldPos.Z;
        Depth = FMath::Max(0.f, Depth);

        constexpr float GrassDepth = 10.f;
        constexpr float SoilDepth = 500.f;
        constexpr float GrassBlend = 3.f;
        constexpr float SoilBlend = 20.f;

        // 与 BiomeFreq 同比收窄（保持 BiomeBlend / BiomeFreq ≈ 150）
        constexpr float BiomeBlend = BiomeFreq * 1200.f;        // WidthCm 是半宽


        const FColor Forest(0, 200, 0);
        const FColor Desert(220, 180, 80);
        const FColor Soil(125, 95, 60);
        const FColor Rock(115, 115, 125);

        const FVector2D P2D((double)WorldPos.X, (double)WorldPos.Y);

        // 大块群系 + 小尺度碎斑（200m 尺度，权重 0.25）
        const float N = FMath::PerlinNoise2D(P2D * BiomeFreq)
            + 0.25f * FMath::PerlinNoise2D(P2D * (BiomeFreq * 10.f));

        const float sBiome = FMath::SmoothStep(-BiomeBlend, BiomeBlend, N);
        FVector3f C = FMath::Lerp(
            FVector3f(Forest.R, Forest.G, Forest.B),
            FVector3f(Desert.R, Desert.G, Desert.B), sBiome);

        const float wGrass = FMath::SmoothStep(GrassDepth - GrassBlend, GrassDepth + GrassBlend, Depth);
        const float wSoil = FMath::SmoothStep(SoilDepth - SoilBlend, SoilDepth + SoilBlend, Depth);

        C = FMath::Lerp(C, FVector3f(Soil.R, Soil.G, Soil.B), wGrass);
        C = FMath::Lerp(C, FVector3f(Rock.R, Rock.G, Rock.B), wSoil);

        return FColor(
            (uint8)FMath::Clamp(FMath::RoundToInt(C.X), 0, 255),
            (uint8)FMath::Clamp(FMath::RoundToInt(C.Y), 0, 255),
            (uint8)FMath::Clamp(FMath::RoundToInt(C.Z), 0, 255),
            255);
    }

#else
    FColor ComputeColor(const FVector3f& WorldPos, const FVector3f& Normal, float SurfaceHeight)
    {
        // 特征尺度约 100m，可见范围内会有多个噪声周期
        constexpr float BiomeFreq = 0.0001f;

        float Depth = SurfaceHeight - WorldPos.Z;
        Depth = FMath::Max(0.f, Depth);

        constexpr float GrassDepth = 10.f;
        constexpr float SoilDepth = 500.f;

        const FColor Forest(0, 200, 0);
        const FColor Desert(220, 180, 80);
        const FColor Soil(125, 95, 60);
        const FColor Rock(115, 115, 125);

        const FVector2D P2D((double)WorldPos.X, (double)WorldPos.Y);
        const float N = FMath::PerlinNoise2D(P2D * BiomeFreq);

        FVector3f C = (N > 0.f)
            ? FVector3f(Desert.R, Desert.G, Desert.B)
            : FVector3f(Forest.R, Forest.G, Forest.B);

        if (Depth > GrassDepth)
        {
            C = FVector3f(Soil.R, Soil.G, Soil.B);
        }
        if (Depth > SoilDepth)
        {
            C = FVector3f(Rock.R, Rock.G, Rock.B);
        }

        return FColor(
            (uint8)FMath::Clamp(FMath::RoundToInt(C.X), 0, 255),
            (uint8)FMath::Clamp(FMath::RoundToInt(C.Y), 0, 255),
            (uint8)FMath::Clamp(FMath::RoundToInt(C.Z), 0, 255),
            255);
    }
#endif
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
    //Out.SurfaceHeights.Reserve(4096);
    Out.Indices.Reserve(8192);
    Out.Colors.Reserve(4096);
    const int32 ChunkN = VoxelConfig::ChunkSize;     // 32
    const int32 EdgeN = VoxelConfig::ChunkStride;   // 33（EdgeCache 用）
    const int32 PaddedN = VoxelConfig::PaddedStride;  // 35（VoxelsBase 用）
    const int32 PaddedNN = PaddedN * PaddedN;          // 1225
    //const int32 S = 1 << LOD;
    const float WorldStep = VoxelSize ;

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
                        // const float Depth = GroundZ - (WorldP.Z + ChunkWorldOrigin.Z);

                        // Out.SurfaceHeights.Add(GroundZ);
                        Out.Positions.Add(WorldP);
                        Out.Normals.Add(Normal);
                        Out.UVs.Add(FVector2f::ZeroVector);
                        Out.Colors.Add(ComputeColor(WorldP + ChunkWorldOrigin, Normal, GroundZ));

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

void FTransvoxelMesher::GenerateRegularPartial(
    const FVoxelChunkData& Chunk,
    float VoxelSize,
    const FVector3f& ChunkWorldOrigin,
    const FIntVector& CellMin,
    const FIntVector& CellMax,
    FVoxelMeshData& InOut)
{
    // 常量提取，避免循环内反复读取
    const int32 PaddedN = VoxelConfig::PaddedStride;
    const int32 PaddedNN = PaddedN * PaddedN;
    const float InvVoxelSize = 1.0f / VoxelSize;
    const float OffX = ChunkWorldOrigin.X;
    const float OffY = ChunkWorldOrigin.Y;
    const float OffZ = ChunkWorldOrigin.Z;

    const int32 ACellX = CellMax.X - CellMin.X + 1;
    const int32 ACellY = CellMax.Y - CellMin.Y + 1;
    const int32 ACellZ = CellMax.Z - CellMin.Z + 1;
    if (ACellX <= 0 || ACellY <= 0 || ACellZ <= 0) return;

    const int32 OldNumIndices = InOut.Indices.Num();
    const int32 OldNumVerts = InOut.Positions.Num();

    // ============ 1 & 2. 过滤与重排（全部裸指针，无函数调用） ============
    if (OldNumIndices == 0)
    {
        InOut.Reset();
    }
    else
    {
        TArray<uint32> KeptIndices;
        KeptIndices.SetNumUninitialized(OldNumIndices);

        TArray<bool> VertexUsed;
        VertexUsed.SetNumZeroed(OldNumVerts);

        const uint32* OldIndices = InOut.Indices.GetData();
        const FVector3f* OldPositions = InOut.Positions.GetData();
        uint32* KeptData = KeptIndices.GetData();
        int32 KeptCount = 0;

        const int32 CMinX = CellMin.X, CMaxX = CellMax.X;
        const int32 CMinY = CellMin.Y, CMaxY = CellMax.Y;
        const int32 CMinZ = CellMin.Z, CMaxZ = CellMax.Z;

        const int32 NumTris = OldNumIndices / 3;
        const uint32* I = OldIndices;
        for (int32 t = 0; t < NumTris; ++t)
        {
            const uint32 I0 = I[0], I1 = I[1], I2 = I[2];
            I += 3;

            const FVector3f& P0 = OldPositions[I0];
            const FVector3f& P1 = OldPositions[I1];
            const FVector3f& P2 = OldPositions[I2];

            // 手写极速 Min
            const float MinX = P0.X < P1.X ? (P0.X < P2.X ? P0.X : P2.X) : (P1.X < P2.X ? P1.X : P2.X);
            const float MinY = P0.Y < P1.Y ? (P0.Y < P2.Y ? P0.Y : P2.Y) : (P1.Y < P2.Y ? P1.Y : P2.Y);
            const float MinZ = P0.Z < P1.Z ? (P0.Z < P2.Z ? P0.Z : P2.Z) : (P1.Z < P2.Z ? P1.Z : P2.Z);

            // floorf 替代 FMath::FloorToInt
            const int32 CX = (int32)floorf((MinX - OffX) * InvVoxelSize + 1e-3f);
            const int32 CY = (int32)floorf((MinY - OffY) * InvVoxelSize + 1e-3f);
            const int32 CZ = (int32)floorf((MinZ - OffZ) * InvVoxelSize + 1e-3f);

            if (CX >= CMinX && CX <= CMaxX &&
                CY >= CMinY && CY <= CMaxY &&
                CZ >= CMinZ && CZ <= CMaxZ)
            {
                continue; // 丢弃
            }

            KeptData[KeptCount++] = I0;
            KeptData[KeptCount++] = I1;
            KeptData[KeptCount++] = I2;

            VertexUsed[I0] = true;
            VertexUsed[I1] = true;
            VertexUsed[I2] = true;
        }
        KeptIndices.SetNum(KeptCount, EAllowShrinking::No);

        // 顶点重排
        TArray<int32> OldToNew;
        OldToNew.SetNumUninitialized(OldNumVerts);
        int32* OldToNewData = OldToNew.GetData();
        for (int32 i = 0; i < OldNumVerts; ++i) OldToNewData[i] = -1;

        TArray<FVector3f> NewPositions;
        TArray<FVector3f> NewNormals;
        TArray<FVector2f> NewUVs;
        TArray<FColor> NewColors;
        //TArray<float>     NewSurfaceHeights;

        NewPositions.SetNumUninitialized(OldNumVerts);
        NewNormals.SetNumUninitialized(OldNumVerts);
        NewUVs.SetNumUninitialized(OldNumVerts);
        NewColors.SetNumUninitialized(OldNumVerts);
        //NewSurfaceHeights.SetNumUninitialized(OldNumVerts);

        FVector3f* NewPosData = NewPositions.GetData();
        FVector3f* NewNormData = NewNormals.GetData();
        FVector2f* NewUVData = NewUVs.GetData();
        FColor* NewColorsData = NewColors.GetData();
        //float* NewHeightData = NewSurfaceHeights.GetData();

        const FVector3f* OldNormals = InOut.Normals.GetData();
        const FVector2f* OldUVs = InOut.UVs.GetData();
        //const float* OldHeights = InOut.SurfaceHeights.GetData();
        const FColor* OldColors = InOut.Colors.GetData();
        int32 NewVertCount = 0;
        for (int32 i = 0; i < OldNumVerts; ++i)
        {
            if (!VertexUsed[i]) continue;
            OldToNewData[i] = NewVertCount;
            NewPosData[NewVertCount] = OldPositions[i];
            NewNormData[NewVertCount] = OldNormals[i];
            NewUVData[NewVertCount] = OldUVs[i];
            //NewHeightData[NewVertCount] = OldHeights[i];
            NewColorsData[NewVertCount] = OldColors[i];
            ++NewVertCount;
        }

        NewPositions.SetNum(NewVertCount, EAllowShrinking::No);
        NewNormals.SetNum(NewVertCount, EAllowShrinking::No);
        NewUVs.SetNum(NewVertCount, EAllowShrinking::No);
        //NewSurfaceHeights.SetNum(NewVertCount, EAllowShrinking::No);
        NewColors.SetNum(NewVertCount, EAllowShrinking::No);
        for (int32 i = 0; i < KeptCount; ++i)
        {
            KeptData[i] = (uint32)OldToNewData[KeptData[i]];
        }

        InOut.Reset();
        InOut.Positions = MoveTemp(NewPositions);
        InOut.Normals = MoveTemp(NewNormals);
        InOut.UVs = MoveTemp(NewUVs);
        // InOut.SurfaceHeights = MoveTemp(NewSurfaceHeights);
        InOut.Indices = MoveTemp(KeptIndices);
        InOut.Colors = MoveTemp(NewColors);
    }

    // ============ 3. 局部 EdgeCache ============
    const int32 LocalN = FMath::Max(ACellX, FMath::Max(ACellY, ACellZ)) + 1;
    const int32 LocalEdgeCount = LocalN * LocalN * LocalN * 3;

    static thread_local TArray<int32> LocalEdgeCache;
    if (LocalEdgeCache.Num() < LocalEdgeCount)
        LocalEdgeCache.SetNumUninitialized(LocalEdgeCount);
    FMemory::Memset(LocalEdgeCache.GetData(), 0xFF, LocalEdgeCount * sizeof(int32));
    int32* EdgeCacheData = LocalEdgeCache.GetData();

    // ============ 4. 暴力预分配（不考虑空间，直接按每 Cell 最大理论值预分配） ============
    const int32 EstCells = ACellX * ACellY * ACellZ;
    const int32 EstVerts = EstCells * 12;
    const int32 EstIndices = EstCells * 15 * 3;

    const int32 OldVerts = InOut.Positions.Num();
    const int32 OldIndicesCount = InOut.Indices.Num();

    InOut.Positions.SetNumUninitialized(OldVerts + EstVerts, EAllowShrinking::No);
    InOut.Normals.SetNumUninitialized(OldVerts + EstVerts, EAllowShrinking::No);
    InOut.UVs.SetNumUninitialized(OldVerts + EstVerts, EAllowShrinking::No);
    InOut.Colors.SetNumUninitialized(OldVerts + EstVerts, EAllowShrinking::No);
    //InOut.SurfaceHeights.SetNumUninitialized(OldVerts + EstVerts, EAllowShrinking::No);
    InOut.Indices.SetNumUninitialized(OldIndicesCount + EstIndices, EAllowShrinking::No);
    FVector3f* PosData = InOut.Positions.GetData();
    FVector3f* NormData = InOut.Normals.GetData();
    FVector2f* UVData = InOut.UVs.GetData();
    //float* HeightData = InOut.SurfaceHeights.GetData();
    uint32* IndicesData = InOut.Indices.GetData();

    FColor* ColorsData = InOut.Colors.GetData();



    int32 CurVertCount = OldVerts;
    int32 CurIndexCount = OldIndicesCount;
    const FVoxel* VoxelsBase = Chunk.Voxels.GetData();
    const float* SurfaceGridData = Chunk.SurfaceGrid.GetData();

    // 静态数组替代 FIntVector 构造
    static const int32 CornerX[8] = { 0,1,0,1,0,1,0,1 };
    static const int32 CornerY[8] = { 0,0,1,1,0,0,1,1 };
    static const int32 CornerZ[8] = { 0,0,0,0,1,1,1,1 };
    constexpr int32 PaddedOffset = VoxelConfig::PaddedOffset;
    const float InvUV = 1.0f / 400.0f;

    // ============ 5. 核心生成（全部裸指针与标量运算） ============
    for (int32 x = CellMin.X; x <= CellMax.X; ++x)
    {
        const int32 XOffset = (x + 1) * PaddedNN;
        for (int32 y = CellMin.Y; y <= CellMax.Y; ++y)
        {
            const int32 YOffset = (y + 1) * PaddedN;
            for (int32 z = CellMin.Z; z <= CellMax.Z; ++z)
            {
                const int32 BaseIdx = XOffset + YOffset + z + 1;

                const int8 W0 = VoxelsBase[BaseIdx].Weight;
                const int8 W1 = VoxelsBase[BaseIdx + PaddedNN].Weight;
                const int8 W2 = VoxelsBase[BaseIdx + PaddedN].Weight;
                const int8 W3 = VoxelsBase[BaseIdx + PaddedNN + PaddedN].Weight;
                const int8 W4 = VoxelsBase[BaseIdx + 1].Weight;
                const int8 W5 = VoxelsBase[BaseIdx + PaddedNN + 1].Weight;
                const int8 W6 = VoxelsBase[BaseIdx + PaddedN + 1].Weight;
                const int8 W7 = VoxelsBase[BaseIdx + PaddedNN + PaddedN + 1].Weight;

                const uint32 Case =
                    ((W0 < 0) ? 0x01u : 0u) | ((W1 < 0) ? 0x02u : 0u) |
                    ((W2 < 0) ? 0x04u : 0u) | ((W3 < 0) ? 0x08u : 0u) |
                    ((W4 < 0) ? 0x10u : 0u) | ((W5 < 0) ? 0x20u : 0u) |
                    ((W6 < 0) ? 0x40u : 0u) | ((W7 < 0) ? 0x80u : 0u);
                if (Case == 0 || Case == 255) continue;

                int8 W[8] = { W0, W1, W2, W3, W4, W5, W6, W7 };

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

                    const int32 AX = CornerX[A], AY = CornerY[A], AZ = CornerZ[A];
                    const int32 BX = CornerX[B], BY = CornerY[B], BZ = CornerZ[B];

                    const int32 StartX = x + (AX < BX ? AX : BX);
                    const int32 StartY = y + (AY < BY ? AY : BY);
                    const int32 StartZ = z + (AZ < BZ ? AZ : BZ);

                    int32 Dir;
                    if (AX != BX)      Dir = 0;
                    else if (AY != BY) Dir = 1;
                    else               Dir = 2;

                    const int32 LX = StartX - CellMin.X;
                    const int32 LY = StartY - CellMin.Y;
                    const int32 LZ = StartZ - CellMin.Z;
                    const int32 EdgeIdxPos = ((LX * LocalN + LY) * LocalN + LZ) * 3 + Dir;
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
                        const float t = (Den > -1e-6f && Den < 1e-6f) ? 0.5f : (-WA) / Den;

                        const float PAX = (float)(x + AX);
                        const float PAY = (float)(y + AY);
                        const float PAZ = (float)(z + AZ);
                        const float PBX = (float)(x + BX);
                        const float PBY = (float)(y + BY);
                        const float PBZ = (float)(z + BZ);

                        const float LPX = PAX + (PBX - PAX) * t;
                        const float LPY = PAY + (PBY - PAY) * t;
                        const float LPZ = PAZ + (PBZ - PAZ) * t;

                        const float WPX = LPX * VoxelSize + OffX;
                        const float WPY = LPY * VoxelSize + OffY;
                        const float WPZ = LPZ * VoxelSize + OffZ;

                        const int32 xi = (int32)(LPX + 0.5f);
                        const int32 yi = (int32)(LPY + 0.5f);
                        const int32 zi = (int32)(LPZ + 0.5f);
                        const int32 CenterIdx = (xi + 1) * PaddedNN + (yi + 1) * PaddedN + (zi + 1);

                        const float dx = (float)VoxelsBase[CenterIdx + PaddedNN].Weight - (float)VoxelsBase[CenterIdx - PaddedNN].Weight;
                        const float dy = (float)VoxelsBase[CenterIdx + PaddedN].Weight - (float)VoxelsBase[CenterIdx - PaddedN].Weight;
                        const float dz = (float)VoxelsBase[CenterIdx + 1].Weight - (float)VoxelsBase[CenterIdx - 1].Weight;

                        const float LenSq = dx * dx + dy * dy + dz * dz;
                        float NX = 0.0f, NY = 0.0f, NZ = 1.0f;
                        if (LenSq >= 1e-6f)
                        {
                            const float InvLen = 1.0f / sqrtf(LenSq);
                            NX = dx * InvLen;
                            NY = dy * InvLen;
                            NZ = dz * InvLen;
                        }

                        const int32 pix = xi + PaddedOffset;
                        const int32 piy = yi + PaddedOffset;
                        const float GroundZ = SurfaceGridData[pix * PaddedN + piy];
                        
                        GlobalIdx = CurVertCount++;

                        // 直接标量赋值，绝不构造临时结构体
                        PosData[GlobalIdx].X = WPX; PosData[GlobalIdx].Y = WPY; PosData[GlobalIdx].Z = WPZ;
                        NormData[GlobalIdx].X = NX; NormData[GlobalIdx].Y = NY; NormData[GlobalIdx].Z = NZ;
                        UVData[GlobalIdx].X = WPX * InvUV; UVData[GlobalIdx].Y = WPY * InvUV;
                        //HeightData[GlobalIdx] = GroundZ ;
                        ColorsData[GlobalIdx] = ComputeColor(PosData[GlobalIdx], NormData[GlobalIdx], GroundZ); // FColor(255, 0, 0);
                        
                        Cached = GlobalIdx;
                    }

                    VertexIndices[VCount++] = GlobalIdx;
                }

                const int32 TCount = CD.GetTriangleCount();
                for (int32 t = 0; t < TCount; ++t)
                {
                    const int32 I0 = CD.vertexIndex[t * 3 + 0];
                    const int32 I1 = CD.vertexIndex[t * 3 + 1];
                    const int32 I2 = CD.vertexIndex[t * 3 + 2];
                    if (I0 >= VCount || I1 >= VCount || I2 >= VCount) continue;

                    IndicesData[CurIndexCount++] = (uint32)VertexIndices[I0];
                    IndicesData[CurIndexCount++] = (uint32)VertexIndices[I2];
                    IndicesData[CurIndexCount++] = (uint32)VertexIndices[I1];
                }
            }
        }
    }

    // ============ 6. 修剪多余空间 ============
    InOut.Positions.SetNum(CurVertCount, EAllowShrinking::No);
    InOut.Normals.SetNum(CurVertCount, EAllowShrinking::No);
    InOut.UVs.SetNum(CurVertCount, EAllowShrinking::No);
    InOut.Colors.SetNum(CurVertCount, EAllowShrinking::No);

    //InOut.SurfaceHeights.SetNum(CurVertCount, EAllowShrinking::No);
    InOut.Indices.SetNum(CurIndexCount, EAllowShrinking::No);
}