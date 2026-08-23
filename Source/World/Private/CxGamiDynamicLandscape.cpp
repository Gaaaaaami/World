// Fill out your copyright notice in the Description page of Project Settings.


#include "CxGamiDynamicLandscape.h"
#include "GamiDynamicNoise.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Kismet/GameplayStatics.h"

#include "CxGlobalData.h"


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
	PrevMS = this->Now();

	if (this->Noise2DTexture)
	{
		auto g = UGameplayStatics::GetGameInstance(this->GetWorld());
		auto GlobalData = g->GetSubsystem<UCxGlobalData>();
		GlobalData->AddNoise(*this->Noise2DTexture);		// make noise data to class of GlobalData 
	}
}
// Called every frame
void ACxGamiDynamicLandscape::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	APlanetActor::GenerateAllChunks();


}
double ACxGamiDynamicLandscape::Now()
{
    return (FPlatformTime::Seconds() * 1000.0);
}
