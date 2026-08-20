// Fill out your copyright notice in the Description page of Project Settings.

#include "World.h"
#include "Modules/ModuleManager.h"



class ShaderInitialize {
public:
    ShaderInitialize(bool InAutoCirle = false)
    {
        // 👇 核心：把项目根目录的 "Shader" 文件夹映射为虚拟路径 "/Project"
        FString ShaderDirectory = FPaths::Combine(FPaths::ProjectDir(), TEXT("Private"));
        // ✅ 关键：映射前先判重，避免热重载崩溃
        if (!AllShaderSourceDirectoryMappings().Contains(TEXT("/Project")))
        {
            AddShaderSourceDirectoryMapping(TEXT("/Project"), ShaderDirectory);
        }

        if (InAutoCirle)
        {
            delete this;
        }
    }

};
static ShaderInitialize ShaderInitializeInstance;

void FWorldModule::StartupModule()
{
    FDefaultGameModuleImpl::StartupModule();
}

void FWorldModule::ShutdownModule()
{
    FDefaultGameModuleImpl::ShutdownModule();
}

IMPLEMENT_PRIMARY_GAME_MODULE(FWorldModule, World, "World");
