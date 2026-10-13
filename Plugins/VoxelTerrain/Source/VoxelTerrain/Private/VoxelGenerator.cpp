#include "VoxelGenerator.h"

// ============================ Fbm ============================
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

// ============================ SampleDensity ============================
float FVoxelGenerator::SampleDensity(const FVector& WorldPos) const
{
    FVector P(WorldPos.X, WorldPos.Y, 0.f);
    const float N = Fbm(P * Settings.NoiseFreq);
    const float SurfaceZ = Settings.BaseWorldZ + N * Settings.HillHeight;

    float Density = SurfaceZ - WorldPos.Z;

    if (Settings.CaveStrength > 0.f)
    {
        const float CaveNoise = Fbm(WorldPos * (Settings.NoiseFreq * 3.f));
        if (FMath::Abs(Density) < Settings.CaveStrength)
        {
            Density -= (Settings.CaveThreshold - FMath::Abs(CaveNoise))
                * Settings.CaveStrength;
        }
    }
    return Density;
}

// ============================ GenerateChunk ============================
void FVoxelGenerator::GenerateChunk(
    int32 ChunkX, int32 ChunkY, int32 ChunkZ,
    int32 LOD,
    FVoxel* OutVoxels) const
{
    const int32 N = VoxelConfig::ChunkStride;
    const int32 S = 1 << LOD;
    const float Step = VoxelConfig::VoxelSize * S;

    const FVector Origin(
        ChunkX * VoxelConfig::ChunkSize * Step,
        ChunkY * VoxelConfig::ChunkSize * Step,
        ChunkZ * VoxelConfig::ChunkSize * Step);

    constexpr float Scale = 200.f;

    for (int32 x = 0; x < N; ++x)
        for (int32 y = 0; y < N; ++y)
            for (int32 z = 0; z < N; ++z)
            {
                const FVector WP = Origin + FVector(x, y, z) * Step;
                const float D = -SampleDensity(WP);
                const float W = FMath::Clamp(D / Scale, -1.f, 1.f);
                OutVoxels[(x * N + y) * N + z].Weight =
                    (FVoxelWeight)FMath::RoundToInt(W * 127.f);
            }
}