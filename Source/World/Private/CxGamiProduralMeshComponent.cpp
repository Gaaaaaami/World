// Fill out your copyright notice in the Description page of Project Settings.


#include "CxGamiProduralMeshComponent.h"
#include "CxDynamicLandScape.h"
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
#if 1
	auto DynamicLandScapeCast = Cast<ACxDynamicLandScape>(this->DynamicLandScape);
	float DistSquared = FVector2D::DistSquared(FVector2D(Location), FVector2D(DynamicLandScapeCast->PlayerLocation));
	if(DistSquared >= DynamicLandScapeCast->LoadedRangeSqared)
	{
		DynamicLandScapeCast->CircleChunk(this->Location);
		return;
	}

	DynamicLandScapeCast->UpdateLOD(this->Location, DistSquared);
#endif
}


