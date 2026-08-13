// Fill out your copyright notice in the Description page of Project Settings.


#include "CxGameMode.h"
void ACxGameMode::StartPlay()
{
	AGameModeBase::StartPlay();
	RHITest = NewObject<URHITest>();
	RHITest->Draw();  // ✅ 调用即绘图

}