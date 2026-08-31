// Fill out your copyright notice in the Description page of Project Settings.


#include "GamiDynamicNoise.h"
#include "Kismet/GameplayStatics.h"
#include "CxGlobalData.h"

UGamiDynamicNoise::UGamiDynamicNoise():UNoiseGenerator()
{
}

UGamiDynamicNoise::~UGamiDynamicNoise()
{

}
float UGamiDynamicNoise::FractalNoise(const FVector& Position) const
{
	float Value = 0.0f;
	float Amplitude = 20.f;
	float Frequency = NoiseScale;

	for (int32 i = 0; i < Octaves; i++)
	{
		Value += FMath::PerlinNoise2D(FVector2D(Position) * Frequency) * Amplitude;
		Frequency *= Lacunarity;
		Amplitude *= Persistence;
	}

	return Value;
}
float UGamiDynamicNoise::SampleDensity(const FVector& WorldPosition) const
{
	// 基础地面高度（你可以设个非零值让地面浮在空中）

	float GroundZ = 200.f;

	// 2D 噪声：只用 X, Y 坐标，Z 不参与噪声采样
	// 这样同一垂直柱子上所有点的噪声值一样 → 形成平坦的层状地形
	FVector NoisePos(WorldPosition.X, WorldPosition.Y, 0.0f);
	float TerrainNoise = FractalNoise(NoisePos);

	// 最终地形高度 = 基础高度 + 噪声扰动
	float TerrainHeight = GroundZ + TerrainNoise * NoiseAmplitude;

	// 密度 = 当前点的 Z 坐标 - 地形高度
	// Z < TerrainHeight → 负值（地下/实心）
	// Z > TerrainHeight → 正值（空气）
	// Z == TerrainHeight → 等值面（Surface Nets 提取这里）
	float FinalDensity = WorldPosition.Z - TerrainHeight;

	return FinalDensity;
}
