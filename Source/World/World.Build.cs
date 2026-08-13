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
            "RHI",
            "ImageWrapper"
        });

		PrivateDependencyModuleNames.AddRange(new string[] {    "Slate",
            "SlateCore"});
        PublicDependencyModuleNames.Add("Core");

        // PrivateIncludePaths.Add(Path.Combine(ModuleDirectory, "Library"));

        PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "..", "..", "Library"));
        // Uncomment if you are using Slate UI
        // PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

        // Uncomment if you are using online features
        // PrivateDependencyModuleNames.Add("OnlineSubsystem");

        // To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true

    }
}
