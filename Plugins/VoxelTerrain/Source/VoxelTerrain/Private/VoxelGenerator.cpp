#include "VoxelGenerator.h"

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

float FVoxelGenerator::SampleDensity(const FVector& WorldPos) const
{
    // 地形高度
    FVector P(WorldPos.X, WorldPos.Y, 0.f);
    const float N = Fbm(P * Settings.NoiseFreq);
    const float SurfaceZ = Settings.BaseWorldZ + N * Settings.HillHeight;

    // 正数 = 在表面以下（实心）
    return SurfaceZ - WorldPos.Z;
}

void FVoxelGenerator::GenerateChunk(int32 ChunkX, int32 ChunkY, int32 ChunkZ,
    FVoxel* OutVoxels) const
{
    const int32 N = VoxelConfig::ChunkStride;
    const float Step = VoxelConfig::VoxelSize;

    // chunk 的世界原点
    const FVector Origin(
        ChunkX * VoxelConfig::ChunkSize * Step,
        ChunkY * VoxelConfig::ChunkSize * Step,
        ChunkZ * VoxelConfig::ChunkSize * Step);

    // 把"世界高度差"映射到 int8 权重 [-127, 127]
    constexpr float Scale = 400.f;   // cm 每单位权重

    int32 SolidCount = 0;
    for (int32 x = 0; x < N; ++x)
        for (int32 y = 0; y < N; ++y)
            for (int32 z = 0; z < N; ++z)
            {
                const FVector WP = Origin + FVector(x, y, z) * Step;
                const float D = SampleDensity(WP);   // >0 = 实心

                // 注意符号：我们要 <0 表示实心，所以取负
                const float W = FMath::Clamp(-D / Scale, -1.f, 1.f);
                FVoxel& V = OutVoxels[(x * N + y) * N + z];
                V.Weight = (int8)FMath::RoundToInt(W * 127.f);

                if (V.IsSolid()) ++SolidCount;
            }
}