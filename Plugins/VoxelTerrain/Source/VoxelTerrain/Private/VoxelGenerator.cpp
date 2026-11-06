#include "VoxelGenerator.h"

// ============================================================
// 全局静态数据定义
// ============================================================
TSharedPtr<TArray<FVoxelGenerator::FNoiseLayerData>> FVoxelGenerator::GlobalNoiseLayers = nullptr;
TSharedPtr<TArray<FVoxelGenerator::FNoiseLayer3DData>> FVoxelGenerator::GlobalNoiseLayers3D = nullptr;

namespace
{
    FORCEINLINE float SmoothMax(float a, float b, float k)
    {
        const float h = FMath::Clamp(0.5f + 0.5f * (b - a) / k, 0.f, 1.f);
        return FMath::Lerp(a, b, h) + k * h * (1.f - h);
    }

#if VOXEL_USE_NOISE_TEXTURE
    FORCEINLINE float SampleTex(const TSharedPtr<TArray<float>>& D, int32 Size, const FVector2D& UV)
    {
        if (Size <= 0 || !D.IsValid()) return 0.5f;
        const float S = (float)Size;

        auto FastWrap = [S](float V) {
            float f = V - FMath::FloorToFloat(V / S) * S;
            return f < 0.f ? f + S : f;
        };

        float fx = FastWrap(UV.X);
        float fy = FastWrap(UV.Y);

        const int32 x0 = (int32)fx; const int32 y0 = (int32)fy;
        const int32 x1 = (x0 + 1) % Size; const int32 y1 = (y0 + 1) % Size;
        const float tx = fx - x0; const float ty = fy - y0;

        const TArray<float>& A = *D;
        return FMath::Lerp(
            FMath::Lerp(A[y0 * Size + x0], A[y0 * Size + x1], tx),
            FMath::Lerp(A[y1 * Size + x0], A[y1 * Size + x1], tx),
            ty
        );
    }

    FORCEINLINE float SampleTex3D(const TSharedPtr<TArray<float>>& D, int32 Size, const FVector& UVW)
    {
        if (Size <= 0 || !D.IsValid()) return 0.5f;
        const float S = (float)Size;

        auto FastWrap = [S](float V) {
            float f = V - FMath::FloorToFloat(V / S) * S;
            return f < 0.f ? f + S : f;
        };

        const float fx = FastWrap(UVW.X);
        const float fy = FastWrap(UVW.Y);
        const float fz = FastWrap(UVW.Z);

        const int32 x0 = (int32)fx; const int32 y0 = (int32)fy; const int32 z0 = (int32)fz;
        const int32 x1 = (x0 + 1) % Size; const int32 y1 = (y0 + 1) % Size; const int32 z1 = (z0 + 1) % Size;

        const float tx = fx - x0; const float ty = fy - y0; const float tz = fz - z0;

        const TArray<float>& A = *D;
        const int32 ZStride = Size * Size;
        auto GetVal = [&](int32 x, int32 y, int32 z) -> float {
            return A[z * ZStride + y * Size + x];
        };

        const float v00 = FMath::Lerp(GetVal(x0, y0, z0), GetVal(x1, y0, z0), tx);
        const float v10 = FMath::Lerp(GetVal(x0, y1, z0), GetVal(x1, y1, z0), tx);
        const float v01 = FMath::Lerp(GetVal(x0, y0, z1), GetVal(x1, y0, z1), tx);
        const float v11 = FMath::Lerp(GetVal(x0, y1, z1), GetVal(x1, y1, z1), tx);

        const float v0 = FMath::Lerp(v00, v10, ty);
        const float v1 = FMath::Lerp(v01, v11, ty);
        return FMath::Lerp(v0, v1, tz);
    }
#endif
}

// ============================================================
// 基础原生噪声
// ============================================================
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

// ============================================================
// 多层贴图噪声叠加
// ============================================================
float FVoxelGenerator::TexFbm2D(const FVector2D& WorldXY) const
{
#if VOXEL_USE_NOISE_TEXTURE
    const TSharedPtr<TArray<FNoiseLayerData>> &Layers = GlobalNoiseLayers;
    if (Layers.IsValid() && Layers->Num() > 0)
    {
        float TotalNoise = 0.f;
        float TotalAmplitude = 0.f;

        for (const FNoiseLayerData& Layer : *Layers)
        {
            if (Layer.Size <= 0 || !Layer.Data.IsValid()) continue;

            const float BaseScale = 0.015f;
            const float FinalFreq = Settings.NoiseFreq * BaseScale * Layer.Frequency;
            const FVector2D UV = WorldXY * FinalFreq * Layer.Size + Layer.Offset;
            const float N = SampleTex(Layer.Data, Layer.Size, UV);
            const float Val = (N * 2.f - 1.f) * Layer.Amplitude;

            TotalNoise += Val;
            TotalAmplitude += Layer.Amplitude;
        }
        if (TotalAmplitude > 0.f) return TotalNoise / TotalAmplitude;
    }
#endif
    return Fbm2D(WorldXY * Settings.NoiseFreq);
}

float FVoxelGenerator::TexFbm3D(const FVector& WorldXYZ) const
{
#if VOXEL_USE_NOISE_TEXTURE
    const TSharedPtr<TArray<FNoiseLayer3DData>> &Layers = GlobalNoiseLayers3D;
    if (Layers.IsValid() && Layers->Num() > 0)
    {
        float TotalNoise = 0.f;
        float TotalAmplitude = 0.f;

        for (const FNoiseLayer3DData& Layer : *Layers)
        {
            if (Layer.Size <= 0 || !Layer.Data.IsValid()) continue;

            const float FinalFreq = 0.015f * Layer.Frequency;
            const FVector UVW = WorldXYZ * (FinalFreq * Layer.Size) + Layer.Offset;

            const float N = SampleTex3D(Layer.Data, Layer.Size, UVW);
            const float Val = (N * 2.f - 1.f) * Layer.Amplitude;

            TotalNoise += Val;
            TotalAmplitude += Layer.Amplitude;
        }
        if (TotalAmplitude > 0.f) return TotalNoise / TotalAmplitude;
    }
#endif
    return Fbm(WorldXYZ);
}

// ============================================================
// ⭐ 密度计算（拆分：SampleDensity / SampleDensityWithSurface）
// ============================================================
float FVoxelGenerator::SampleDensity(const FVector& WorldPos) const
{
    // 外部调用（ApplyEdit）：自己算一次 SurfaceZ
    const FVector2D P2D(WorldPos.X, WorldPos.Y);
    const float SurfaceNoise = TexFbm2D(P2D);
    const float SurfaceZ = Settings.BaseWorldZ + SurfaceNoise * Settings.HillHeight;
    return SampleDensityWithSurface(WorldPos, SurfaceZ);
}

float FVoxelGenerator::SampleDensityWithSurface(const FVector& WorldPos, float SurfaceZ) const
{
    const float SurfaceDensity = SurfaceZ - WorldPos.Z;
    if (SurfaceDensity <= 0.f) return SurfaceDensity;

    // ⭐ 洞穴 Mask：用廉价的原生 Perlin，省一次昂贵的 3D 贴图
    const FVector MaskP = WorldPos * (Settings.NoiseFreq * 0.25f);
    const float MaskNoise = FMath::PerlinNoise3D(MaskP);
    const float MaskThreshold = Settings.CaveThreshold;
    const float MaskBlend = 0.1f;

    const float t = FMath::Clamp((MaskNoise - (MaskThreshold - MaskBlend)) / (2.f * MaskBlend), 0.f, 1.f);
    const float CaveWeight = t * t * (3.f - 2.f * t);

    if (CaveWeight <= 0.f) return SurfaceDensity;

    // ⭐ 洞穴形状：查两次 3D 贴图
    const FVector CaveP = WorldPos * (Settings.NoiseFreq * 0.7f);
    const float A = TexFbm3D(CaveP);
    const float B = TexFbm3D(CaveP + FVector(317.0f, 173.0f, 97.0f));

    const float BaseRadius = 0.15f;
    const float EffectiveRadius = BaseRadius * CaveWeight;

    const float DistA = FMath::Abs(A) - EffectiveRadius;
    const float DistB = FMath::Abs(B) - EffectiveRadius;
    const float Dist = SmoothMax(DistA, DistB, 0.05f);

    const float CaveFreq = Settings.NoiseFreq * 0.7f;
    const float NoiseToWorldScale = 1.0f / (CaveFreq + 1e-5f);
    const float K_Noise = 1.2f;

    const float CaveDensity = Dist * NoiseToWorldScale * K_Noise;
    return FMath::Min(SurfaceDensity, CaveDensity);
}

// ============================================================
// 体素数据生成（⭐ 2D 噪声缓存优化版）
// ============================================================
void FVoxelGenerator::GenerateChunk(
    int32 ChunkX, int32 ChunkY, int32 ChunkZ,
    int32 LOD,
    const FVoxelModificationMap& Modifications,
    FVoxel* OutVoxels) const
{
    check(LOD >= 0 && OutVoxels != nullptr);

    const int32 N = VoxelConfig::PaddedStride;
    const int32 S = 1 << LOD;
    const float Step = VoxelConfig::VoxelSize * S;

    const FVector Origin(
        ChunkX * VoxelConfig::ChunkSize * Step,
        ChunkY * VoxelConfig::ChunkSize * Step,
        ChunkZ * VoxelConfig::ChunkSize * Step);

    constexpr float Scale = 150.f;
    constexpr float InvScale = 1.f / Scale;
    const float InvVoxelSize = 1.f / VoxelConfig::VoxelSize;

    // ⭐ 分支提升：整个 chunk 只判断一次
    const bool bHasMods = Modifications.Num() > 0;

    const float StartX = Origin.X - VoxelConfig::PaddedOffset * Step;
    const float StartY = Origin.Y - VoxelConfig::PaddedOffset * Step;
    const float StartZ = Origin.Z - VoxelConfig::PaddedOffset * Step;

    for (int32 ix = 0; ix < N; ++ix)
    {
        const float WX = StartX + ix * Step;

        for (int32 iy = 0; iy < N; ++iy)
        {
            const float WY = StartY + iy * Step;

            // ⭐ 核心优化：(X, Y) 的 2D 噪声只算一次（原版每 voxel 一次）
            const float SurfaceNoise = TexFbm2D(FVector2D(WX, WY));
            const float SurfaceZ = Settings.BaseWorldZ + SurfaceNoise * Settings.HillHeight;

            for (int32 iz = 0; iz < N; ++iz)
            {
                const float WZ = StartZ + iz * Step;
                const FVector WP(WX, WY, WZ);

                const float D = -SampleDensityWithSurface(WP, SurfaceZ);
                const float W = FMath::Clamp(D * InvScale, -1.f, 1.f);
                int8 FinalWeight = (FVoxelWeight)FMath::RoundToInt(W * 127.f);

                if (bHasMods)
                {
                    const FIntVector LOD0VoxelCoord(
                        FMath::FloorToInt(WP.X * InvVoxelSize + 0.5f),
                        FMath::FloorToInt(WP.Y * InvVoxelSize + 0.5f),
                        FMath::FloorToInt(WP.Z * InvVoxelSize + 0.5f));
                    if (const FVoxelWeight* M = Modifications.Find(LOD0VoxelCoord))
                        FinalWeight = *M;
                }

                OutVoxels[(ix * N + iy) * N + iz].Weight = FinalWeight;
            }
        }
    }
}