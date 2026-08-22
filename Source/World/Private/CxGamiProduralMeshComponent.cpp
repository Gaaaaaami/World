// Fill out your copyright notice in the Description page of Project Settings.


#include "CxGamiProduralMeshComponent.h"
#include <cmath>

UCxGamiProduralMeshComponent::UCxGamiProduralMeshComponent(const FObjectInitializer& ObjectInitializer):UProceduralMeshComponent(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
}

UCxGamiProduralMeshComponent::~UCxGamiProduralMeshComponent()
{

}

void UCxGamiProduralMeshComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{

	UProceduralMeshComponent::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if ((FPlatformTime::Seconds() * 1000.0) - PrevMS > 32.0)
	{

		FVector AbsLocation = this->Location * this->PlanetActor->ChunkSize;
		float dist = FVector::DistSquared(AbsLocation, this->PlanetActor->PlayerLocation);
		float Alpha = dist / this->PlanetActor->LoadRadiu;

		if (Alpha > 1.f)
		{
			this->PlanetActor->CircleChunk(Location);
		}
		PrevMS = (FPlatformTime::Seconds() * 1000.0);
	}
}


void UCxGamiProduralMeshComponent::BindPlanetActor(APlanetActor* InPlanetActor)
{
	this->PlanetActor = InPlanetActor;
}

