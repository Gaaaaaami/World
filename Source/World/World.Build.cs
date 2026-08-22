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
            //"Kismet",
            "RenderCore",
            "RHI"
        });

		PrivateDependencyModuleNames.AddRange(new string[] {    "Slate",
            "SlateCore"});
        PublicDependencyModuleNames.Add("Core");

        // PrivateIncludePaths.Add(Path.Combine(ModuleDirectory, "Library"));

        PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "..", "..", "Library"));
        //string ThirdPartyPath = Path.Combine(ModuleDirectory, "Private/ThirdParty/FastNoiseSIMD");
        //PublicIncludePaths.Add(ThirdPartyPath);


    }
}
