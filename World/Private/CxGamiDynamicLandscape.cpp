// Fill out your copyright notice in the Description page of Project Settings.


#include "CxGamiDynamicLandscape.h"
#include "GamiDynamicNoise.h"
// Sets default values


ACxGamiDynamicLandscape::ACxGamiDynamicLandscape():APlanetActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

ACxGamiDynamicLandscape::~ACxGamiDynamicLandscape()
{

}

// Called when the game starts or when spawned
void ACxGamiDynamicLandscape::BeginPlay()
{
	Super::BeginPlay();
	RHIDT = NewObject<URHIDensityTools>();
	RHIDT->Init(18);
	RHIDT->ReadCallBackFunction = [this](TArray<TArray<float>> buffers, TArray<FVector3f> Locations, TArray<FVector> ChunkCenters) {
			for (int i = 0; i < buffers.Num(); i++)
			{
				this->GenerateMesh(buffers[i], Locations[i], ChunkCenters[i]);
			}
		};

	RuntimeMSecond = Now();
}
double ACxGamiDynamicLandscape::Now()
{
	return FPlatformTime::Seconds() * 1000.0;
}

// Called every frame
void ACxGamiDynamicLandscape::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	double CurrentMSTime = Now();
	if (CurrentMSTime - RuntimeMSecond > 32)
	{
		APlanetActor::GenerateAllChunks(0, 0, RHIDT);
		RHIDT->Run();		/** < ÂÖÑ¯äÖÈ¾Ïß³Ì */
		RuntimeMSecond = CurrentMSTime;
	}
}
