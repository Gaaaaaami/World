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

}

// Called every frame
void ACxGamiDynamicLandscape::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}
