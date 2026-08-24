// Fill out your copyright notice in the Description page of Project Settings.


#include "CxGlobalData.h"
#include "Kismet/GameplayStatics.h"

TArray<TTuple<TArray<float>, int, int>>& GlobalNoiseContainer() {
    static 	TArray<TTuple<TArray<float>, int, int>> instance;
    return instance;
};

void UCxGlobalData::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UCxGlobalData::Deinitialize()
{
	Super::Deinitialize();
}

FVector2D UCxGlobalData::PositionToUV(const FVector& Position, float WorldSize, bool bTile)
{
    float InvSize = 1.0f / WorldSize;
    float U = Position.X * InvSize;
    float V = Position.Y * InvSize;  // Y 对应 V（Z-up 体系下水平面是 XY）

    if (bTile)
    {
        // 平铺：超出 0~1 循环重复
        U = FMath::Frac(U);
        V = FMath::Frac(V);
    }
    else
    {
        // 不平铺：Clamp 到 0~1
        U = FMath::Clamp(U, 0.0f, 1.0f);
        V = FMath::Clamp(V, 0.0f, 1.0f);
    }

    return FVector2D(U, V);
}
void UCxGlobalData::AddNoise(UTexture2D& t)
{
#if 1
    auto PlatformData = t.GetPlatformData();
    auto CPUCopy = t.GetCPUCopy();
    auto Format = CPUCopy->Format;
    auto Width = CPUCopy->GetWidth();
    auto Height = CPUCopy->GetHeight();
    if (Format == ERawImageFormat::BGRA8)
    {
        TArray<float> noise;
        noise.SetNumUninitialized(Width * Height);
        for (int i = 0; i < Width; i++)
        {
            for (int j = 0; j < Height; j++)
            {
                int Index = j * Width + i;
                int IndexRGBA = j * (Width * 4) + (i * 4);
                float n = CPUCopy->RawData[IndexRGBA];
                n = n / 255.f;
                noise[Index] = n;
            }
        }
        TTuple<TArray<float>, int, int> data(noise, Width, Height);
        GlobalNoiseContainer().Add(data);
    }
#endif
}

float UCxGlobalData::GetNoise(int32 index, FVector Position)
{
    
    return 0.f;
}
