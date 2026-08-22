// Fill out your copyright notice in the Description page of Project Settings.


#include "CxGamiDynamicLandscape.h"
#include "GamiDynamicNoise.h"
#include "Kismet/KismetRenderingLibrary.h"

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
#if 0
    TArray<float> OutMask;  // 单通道，1字节/像素
   
    FTextureSource& Source = Noise2DTexture->Source;

    int32 Width = Source.GetSizeX();
    int32 Height = Source.GetSizeY();
    int32 NumPixels = Width * Height;

    void* MipData = Source.LockMip(0);

    // 预分配
    OutMask.SetNumUninitialized(NumPixels);

    // 按 FColor 解析，只取 R 通道（遮罩数据通常在 R）
    const FColor* Colors = static_cast<const FColor*>(MipData);
    for (int32 i = 0; i < NumPixels; i++)
    {
        OutMask[i] = Colors[i].R / 255.f; 
    }

    Source.UnlockMip(0);
#else

    if (Noise2DTexture)
    {
        auto PlatformData = Noise2DTexture->GetPlatformData();
        auto Mip = PlatformData->Mips[0];
        const int32 Width = Mip.SizeX;
        const int32 Hieght = Mip.SizeY;
        auto CPUCopy = Noise2DTexture->GetCPUCopy();
        auto Format = CPUCopy->Format; // ← 直接访问成员变量
        return;
    }

#endif
}

// Called every frame
void ACxGamiDynamicLandscape::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (this->Now() - PrevMS > 32)
	{
		APlanetActor::GenerateAllChunks();
        PrevMS = this->Now();
	}
}
double ACxGamiDynamicLandscape::Now()
{
    return (FPlatformTime::Seconds() * 1000.0);
}
