// Fill out your copyright notice in the Description page of Project Settings.


#include "GamiDynamicNoise.h"

UGamiDynamicNoise::UGamiDynamicNoise():UNoiseGenerator()
{

}

UGamiDynamicNoise::~UGamiDynamicNoise()
{

}

float UGamiDynamicNoise::SampleDensity(const FVector& WorldPosition) const
{

#if 0
		// 基础地面高度（你可以设个非零值让地面浮在空中）
	//UE_LOG(LogTemp, Log, TEXT("X:%f, Y:%f, Z%f"), WorldPosition.X, WorldPosition.Y, WorldPosition.Z);

	float GroundZ = 0.0f;

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

	///return 1.f;
	return FinalDensity;
#else

	static float worldZ = 0;//WorldPosition.Z;

	if (worldZ != WorldPosition.Z)
	{
		worldZ = WorldPosition.Z;
	}
	if (WorldPosition.Z < 200.f)
	{
		return -1.f;
	}
	else
		return 1.f;
#endif
}
