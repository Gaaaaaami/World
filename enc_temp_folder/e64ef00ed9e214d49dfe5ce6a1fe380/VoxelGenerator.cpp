#include "VoxelGenerator.h"

// ============================ Smooth Max ============================
// 用 smooth max 代替 max，避免两条等值面相交处出现折痕
namespace
{
    FORCEINLINE float SmoothMax(float a, float b, float k)
    {
        const float h = FMath::Clamp(0.5f + 0.5f * (b - a) / k, 0.f, 1.f);
        return FMath::Lerp(a, b, h) + k * h * (1.f - h);
    }
}

// ============================ Fbm 3D ============================
float FVoxelGenerator::Fbm(const FVector& P) const
{
    float Sum = 0.f;
    float Amp = 1.f;
    float Freq = 1.f;
    float Norm = 0.f;
    const FVector Offset(
        Settings.Seed * 13.37f,
        Settings.Seed * 7.77f,
        Settings.Seed * 3.21f);

    for (int32 i = 0; i < Settings.Octaves; ++i)
    {
        Sum += Amp * FMath::PerlinNoise3D(P * Freq + Offset);
        Norm += Amp;
        Amp *= Settings.Persistence;
        Freq *= Settings.Lacunarity;
    }
    return Norm > 0.f ? Sum / Norm : 0.f;
}

// ============================ Fbm 2D ============================
float FVoxelGenerator::Fbm2D(const FVector2D& P) const
{
    float Sum = 0.f;
    float Amp = 1.f;
    float Freq = 1.f;
    float Norm = 0.f;
    const FVector2D Offset(
        Settings.Seed * 13.37f,
        Settings.Seed * 7.77f);

    for (int32 i = 0; i < Settings.Octaves; ++i)
    {
        Sum += Amp * FMath::PerlinNoise2D(P * Freq + Offset);
        Norm += Amp;
        Amp *= Settings.Persistence;
        Freq *= Settings.Lacunarity;
    }
    return Norm > 0.f ? Sum / Norm : 0.f;
}

// ============================ SampleDensity ============================
float FVoxelGenerator::SampleDensity(const FVector& WorldPos) const
{
    // ===== 1) 2D 地表 =====
    const FVector2D P2D(WorldPos.X, WorldPos.Y);
    const float SurfaceNoise = Fbm2D(P2D * Settings.NoiseFreq);
    const float SurfaceZ = Settings.BaseWorldZ + SurfaceNoise * Settings.HillHeight;

    const float SurfaceDensity = SurfaceZ - WorldPos.Z;

    // 地表以上：不参与洞穴计算
    if (SurfaceDensity <= 0.f)
    {
        return SurfaceDensity;
    }

#if 0
    // ===== 纯 2D 地形测试 =====
    return SurfaceDensity;
#else
    // ===== 2) 低频 mask：平滑控制洞穴权重 =====
    const FVector MaskP = WorldPos * (Settings.NoiseFreq * 0.25f);
    const float MaskNoise = Fbm(MaskP);

    const float MaskThreshold = 0.15f;
    const float MaskBlend = 0.1f;

    // [Threshold - Blend, Threshold + Blend] 之间平滑过渡 0 → 1
    const float t = FMath::Clamp(
        (MaskNoise - (MaskThreshold - MaskBlend)) / (2.f * MaskBlend),
        0.f, 1.f);

    // smoothstep
    const float CaveWeight = t * t * (3.f - 2.f * t);

    if (CaveWeight <= 0.f)
    {
        return SurfaceDensity;
    }

    // ===== 3) 3D 洞穴：双等值面相交 =====
    const FVector CaveP = WorldPos * (Settings.NoiseFreq * 0.7f);

    const float A = Fbm(CaveP);
    const float B = Fbm(CaveP + FVector(317.0f, 173.0f, 97.0f));

    // 半径按 mask 权重缩放，实现平滑变粗
    const float BaseRadius = 0.15f;
    const float EffectiveRadius = BaseRadius * CaveWeight;

    const float DistA = FMath::Abs(A) - EffectiveRadius;
    const float DistB = FMath::Abs(B) - EffectiveRadius;
    const float Dist = SmoothMax(DistA, DistB, 0.05f);

    const float CaveDensity = Dist * 100.f;

    return FMath::Min(SurfaceDensity, CaveDensity);
#endif
}

// ============================ GenerateChunk ============================
void FVoxelGenerator::GenerateChunk(
    int32 ChunkX, int32 ChunkY, int32 ChunkZ,
    int32 LOD,
    FVoxel* OutVoxels) const
{
    check(LOD >= 0);
    check(OutVoxels != nullptr);

    const int32 N = VoxelConfig::ChunkStride;
    const int32 S = 1 << LOD;
    const float Step = VoxelConfig::VoxelSize * S;

    const FVector Origin(
        ChunkX * VoxelConfig::ChunkSize * Step,
        ChunkY * VoxelConfig::ChunkSize * Step,
        ChunkZ * VoxelConfig::ChunkSize * Step);

    // 量化尺度：150 约 1.2 世界单位/级
    // 若等值面有锯齿，可降到 100（约 0.8 世界单位/级）
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
                OutVoxels[(x * N + y) * N + z].Weight =
                    (FVoxelWeight)FMath::RoundToInt(W * 127.f);
            }
        }
    }
}