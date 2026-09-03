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
	if (!this->Density.IsEmpty())
	{
		int index = WorldPosition.Z * (this->DensitySize.X * this->DensitySize.Y) + WorldPosition.Y * this->DensitySize.X + WorldPosition.X;
		return this->Density[index];

	}
	return 1.f;
}
