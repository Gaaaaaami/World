// Fill out your copyright notice in the Description page of Project Settings.

#include "World.h"
#include "Modules/ModuleManager.h"


void FWorldModule::StartupModule()
{

    FDefaultGameModuleImpl::StartupModule();

#if 1
    // 👇 核心：把项目根目录的 "Shader" 文件夹映射为虚拟路径 "/Project"
    FString ShaderDirectory = FPaths::Combine(FPaths::ProjectDir(), TEXT("Private"));
    // ✅ 关键：映射前先判重，避免热重载崩溃
    if (!AllShaderSourceDirectoryMappings().Contains(TEXT("/Project")))
    {
        AddShaderSourceDirectoryMapping(TEXT("/Project"), ShaderDirectory);
        UE_LOG(LogTemp, Log, TEXT("[World] Shader directory mapped: %s"), *ShaderDirectory);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[World] /Project already mapped, skip"));
    }
#endif
}

void FWorldModule::ShutdownModule()
{
    FDefaultGameModuleImpl::ShutdownModule();
}
IMPLEMENT_PRIMARY_GAME_MODULE(FWorldModule, World, "World");
