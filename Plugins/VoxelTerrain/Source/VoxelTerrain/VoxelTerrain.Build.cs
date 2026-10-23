using UnrealBuildTool;

public class VoxelTerrain : ModuleRules
{
    public VoxelTerrain(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[] {
            "Core",
            "CoreUObject",
            "Engine",
            "RenderCore",
            "RHI",
            "InputCore",  
            "Projects",
            "ProceduralMeshComponent"     // ← 新增
        });

        // 没有额外 Private 依赖
        // MD_Surface 来自 Engine 模块的 MaterialDomain.h
    }
}