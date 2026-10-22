// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeVoxelTerrain_init() {}
static_assert(!UE_WITH_CONSTINIT_UOBJECT, "This generated code can only be compiled with !UE_WITH_CONSTINIT_OBJECT");
	static FPackageRegistrationInfo Z_Registration_Info_UPackage__Script_VoxelTerrain;
	FORCENOINLINE UPackage* Z_Construct_UPackage__Script_VoxelTerrain(ETypeConstructPhase)
	{
		if (!Z_Registration_Info_UPackage__Script_VoxelTerrain.OuterSingleton)
		{
		static const UECodeGen_Private::FPackageParams PackageParams = {
			"/Script/VoxelTerrain",
			nullptr,
			0,
			PKG_CompiledIn | 0x00000000,
			0x6C9EDA44,
			0x616E3623,
			METADATA_PARAMS(0, nullptr)
		};
		UECodeGen_Private::ConstructUPackage(Z_Registration_Info_UPackage__Script_VoxelTerrain.OuterSingleton, PackageParams);
	}
	return Z_Registration_Info_UPackage__Script_VoxelTerrain.OuterSingleton;
}
static FRegisterCompiledInInfo Z_CompiledInDeferPackage_UPackage__Script_VoxelTerrain(Z_Construct_UPackage__Script_VoxelTerrain, TEXT("/Script/VoxelTerrain"), Z_Registration_Info_UPackage__Script_VoxelTerrain, CONSTRUCT_RELOAD_VERSION_INFO(FPackageReloadVersionInfo, 0x6C9EDA44, 0x616E3623));
PRAGMA_ENABLE_DEPRECATION_WARNINGS
