// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;
using System.IO;   // ✅ 必须加，否则 Path 不认识

public class World : ModuleRules
{
	public World(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] {
            "Core", 
            "CoreUObject", 
            "Engine", 
            "InputCore", 
            "ProceduralMeshComponent", 
            "Kismet",
            "RenderCore",
            "RHI"
        });

		PrivateDependencyModuleNames.AddRange(new string[] {    "Slate",
            "SlateCore"});
        PublicDependencyModuleNames.Add("Core");



        // 2. 关键：指定Brutus的路径
        // ModuleDirectory 是当前 World.Build.cs 所在的目录（Source/World）
        string ProjectRoot = Path.GetFullPath(Path.Combine(ModuleDirectory, "../.."));
        string BrutusIncludePath = Path.Combine(ProjectRoot, "Brutus/include/");

        // 3. 用 SystemIncludePaths！UBT不会扫描这里的源文件，只用来#include
        PublicSystemIncludePaths.Add(BrutusIncludePath);
        BrutusIncludePath = Path.Combine(ProjectRoot, "PolyVox/");
        PublicSystemIncludePaths.Add(BrutusIncludePath);





        // Uncomment if you are using Slate UI
        // PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

        // Uncomment if you are using online features
        // PrivateDependencyModuleNames.Add("OnlineSubsystem");

        // To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
    }
}
