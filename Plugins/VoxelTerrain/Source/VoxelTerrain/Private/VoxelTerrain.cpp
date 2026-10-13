// Copyright Epic Games, Inc. All Rights Reserved.

#include "VoxelTerrain.h"
#include "Transvoxel.h"
#define LOCTEXT_NAMESPACE "FVoxelTerrainModule"

void FVoxelTerrainModule::StartupModule()
{
	// 临时验证：引用 Transvoxel 表，确保它被编译
	const int32 TestSize = sizeof(regularCellClass);
	UE_LOG(LogTemp, Log, TEXT("VoxelTerrain: Transvoxel tables loaded, regularCellClass size = %d"), TestSize);
}

void FVoxelTerrainModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FVoxelTerrainModule, VoxelTerrain)