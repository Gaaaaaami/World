#include "VoxelGenerator.h"

namespace
{
    FORCEINLINE float SmoothMax(float a, float b, float k)
    {
        const float h = FMath::Clamp(0.5f + 0.5f * (b - a) / k, 0.f, 1.f);
        return FMath::Lerp(a, b, h) + k * h * (1.f - h);
    }
}

float FVoxelGenerator::Fbm(const FVector& P) const
{
    float Sum = 0.f;
    float Amp = 1.f;
    float Freq = 1.f;
    float Norm = 0.f;
    const FVector Offset(Settings.Seed * 13.37f, Settings.Seed * 7.77f, Settings.Seed * 3.21f);

    for (int32 i = 0; i < Settings.Octaves; ++i)
    {
        Sum += Amp * FMath::PerlinNoise3D(P * Freq + Offset);
        Norm += Amp;
        Amp *= Settings.Persistence;
        Freq *= Settings.Lacunarity;
    }
    return Norm > 0.f ? Sum / Norm : 0.f;
}

float FVoxelGenerator::Fbm2D(const FVector2D& P) const
{
    float Sum = 0.f;
    float Amp = 1.f;
    float Freq = 1.f;
    float Norm = 0.f;
    const FVector2D Offset(Settings.Seed * 13.37f, Settings.Seed * 7.77f);

    for (int32 i = 0; i < Settings.Octaves; ++i)
    {
        Sum += Amp * FMath::PerlinNoise2D(P * Freq + Offset);
        Norm += Amp;
        Amp *= Settings.Persistence;
        Freq *= Settings.Lacunarity;
    }
    return Norm > 0.f ? Sum / Norm : 0.f;
}

float FVoxelGenerator::SampleDensity(const FVector& WorldPos) const
{
    const FVector2D P2D(WorldPos.X, WorldPos.Y);
    const float SurfaceNoise = Fbm2D(P2D * Settings.NoiseFreq);
    const float SurfaceZ = Settings.BaseWorldZ + SurfaceNoise * Settings.HillHeight;

    const float SurfaceDensity = SurfaceZ - WorldPos.Z;

    if (SurfaceDensity <= 0.f) return SurfaceDensity;

    const FVector MaskP = WorldPos * (Settings.NoiseFreq * 0.25f);
    const float MaskNoise = Fbm(MaskP);
    const float MaskThreshold = Settings.CaveThreshold;
    const float MaskBlend = 0.1f;

    const float t = FMath::Clamp((MaskNoise - (MaskThreshold - MaskBlend)) / (2.f * MaskBlend), 0.f, 1.f);
    const float CaveWeight = t * t * (3.f - 2.f * t);

    if (CaveWeight <= 0.f) return SurfaceDensity;

    const FVector CaveP = WorldPos * (Settings.NoiseFreq * 0.7f);
    const float A = Fbm(CaveP);
    const float B = Fbm(CaveP + FVector(317.0f, 173.0f, 97.0f));

    const float BaseRadius = 0.15f;
    const float EffectiveRadius = BaseRadius * CaveWeight;

    const float DistA = FMath::Abs(A) - EffectiveRadius;
    const float DistB = FMath::Abs(B) - EffectiveRadius;
    const float Dist = SmoothMax(DistA, DistB, 0.05f);

    const float CaveDensity = Dist * 100.f;
    return FMath::Min(SurfaceDensity, CaveDensity);
}

void FVoxelGenerator::GenerateChunk(
    int32 ChunkX, int32 ChunkY, int32 ChunkZ,
    int32 LOD,
    const FVoxelModificationMap& Modifications,
    FVoxel* OutVoxels) const
{
    check(LOD >= 0);
    check(OutVoxels != nullptr);

    const int32 N = VoxelConfig::GetChunkStride(LOD);
    const float Step = VoxelConfig::GetVoxelSize(LOD);
    const float WorldSize = VoxelConfig::ChunkWorldSize;

    const FVector Origin(
        ChunkX * WorldSize,
        ChunkY * WorldSize,
        ChunkZ * WorldSize);

    constexpr float Scale = 150.f;

    for (int32 x = 0; x < N; ++x)
    {
        for (int32 y = 0; y < N; ++y)
        {
            for (int32 z = 0; z < N; ++z)
            {
                const FVector WP = Origin + FVector(x, y, z) * Step;
                const float D = -SampleDensity(WP);
                const float W = FMath::Clamp(D / Scale, -1.f, 1.f);
                int8 FinalWeight = (FVoxelWeight)FMath::RoundToInt(W * 127.f);

                if (Modifications.Num() > 0)
                {
                    // ⭐ 核心修复 2：LOD0 单点精确采样，LOD1/LOD2 使用邻域采样
                    if (LOD == 0)
                    {
                        const FIntVector LOD0VoxelCoord(
                            FMath::FloorToInt((WP.X / VoxelConfig::BaseVoxelSize) + 0.5f),
                            FMath::FloorToInt((WP.Y / VoxelConfig::BaseVoxelSize) + 0.5f),
                            FMath::FloorToInt((WP.Z / VoxelConfig::BaseVoxelSize) + 0.5f)
                        );
                        if (const FVoxelWeight* ModifiedWeight = Modifications.Find(LOD0VoxelCoord))
                        {
                            FinalWeight = *ModifiedWeight;
                        }
                    }
                    else
                    {
                        const int32 L0X = FMath::FloorToInt(WP.X / VoxelConfig::BaseVoxelSize + 0.5f);
                        const int32 L0Y = FMath::FloorToInt(WP.Y / VoxelConfig::BaseVoxelSize + 0.5f);
                        const int32 L0Z = FMath::FloorToInt(WP.Z / VoxelConfig::BaseVoxelSize + 0.5f);

                        int8 BestWeight = FinalWeight;
                        bool bFound = false;

                        // 搜索周围 3x3x3 的 LOD0 体素，只要范围内有修改，就采纳
                        for (int32 ox = -1; ox <= 1; ++ox) {
                            for (int32 oy = -1; oy <= 1; ++oy) {
                                for (int32 oz = -1; oz <= 1; ++oz) {
                                    FIntVector LOD0VoxelCoord(L0X + ox, L0Y + oy, L0Z + oz);
                                    if (const FVoxelWeight* WMod = Modifications.Find(LOD0VoxelCoord))
                                    {
                                        if (!bFound) {
                                            BestWeight = *WMod;
                                            bFound = true;
                                        }
                                        else {
                                            // 挖掘(正)取更大值，建造(负)取更小值(绝对值更大)
                                            if (FMath::Abs(*WMod) > FMath::Abs(BestWeight)) {
                                                BestWeight = *WMod;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        if (bFound)
                        {
                            FinalWeight = BestWeight;
                        }
                    }
                }

                OutVoxels[(x * N + y) * N + z].Weight = FinalWeight;
            }
        }
    }
}