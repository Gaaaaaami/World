// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

// IWYU pragma: private, include "VoxelTerrainComponent.h"

#ifdef VOXELTERRAIN_VoxelTerrainComponent_generated_h
#error "VoxelTerrainComponent.generated.h already included, missing '#pragma once' in VoxelTerrainComponent.h"
#endif
#define VOXELTERRAIN_VoxelTerrainComponent_generated_h

#include "UObject/ObjectMacros.h"
#include "UObject/ReflectedTypeAccessors.h"
#include "UObject/ScriptMacros.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS

// ********** Begin ScriptStruct FVoxelNoiseLayerInfo **********************************************
struct Z_Construct_UScriptStruct_FVoxelNoiseLayerInfo_Statics;
VOXELTERRAIN_API UScriptStruct* Z_Construct_UScriptStruct_FVoxelNoiseLayerInfo(ETypeConstructPhase);

#define FID_Users_Administrator_Documents_Unreal_Projects_World_Plugins_VoxelTerrain_Source_VoxelTerrain_Public_VoxelTerrainComponent_h_21_GENERATED_BODY \
	friend struct ::Z_Construct_UScriptStruct_FVoxelNoiseLayerInfo_Statics; \
	UE_NODEBUG static UScriptStruct* StaticStruct() { return Z_Construct_UScriptStruct_FVoxelNoiseLayerInfo(ETypeConstructPhase::Inner); }


struct FVoxelNoiseLayerInfo;
// ********** End ScriptStruct FVoxelNoiseLayerInfo ************************************************

// ********** Begin ScriptStruct FVoxelNoiseLayer3DInfo ********************************************
struct Z_Construct_UScriptStruct_FVoxelNoiseLayer3DInfo_Statics;
VOXELTERRAIN_API UScriptStruct* Z_Construct_UScriptStruct_FVoxelNoiseLayer3DInfo(ETypeConstructPhase);

#define FID_Users_Administrator_Documents_Unreal_Projects_World_Plugins_VoxelTerrain_Source_VoxelTerrain_Public_VoxelTerrainComponent_h_39_GENERATED_BODY \
	friend struct ::Z_Construct_UScriptStruct_FVoxelNoiseLayer3DInfo_Statics; \
	UE_NODEBUG static UScriptStruct* StaticStruct() { return Z_Construct_UScriptStruct_FVoxelNoiseLayer3DInfo(ETypeConstructPhase::Inner); }


struct FVoxelNoiseLayer3DInfo;
// ********** End ScriptStruct FVoxelNoiseLayer3DInfo **********************************************

// ********** Begin ScriptStruct FVoxelVegetationConfig ********************************************
struct Z_Construct_UScriptStruct_FVoxelVegetationConfig_Statics;
VOXELTERRAIN_API UScriptStruct* Z_Construct_UScriptStruct_FVoxelVegetationConfig(ETypeConstructPhase);

#define FID_Users_Administrator_Documents_Unreal_Projects_World_Plugins_VoxelTerrain_Source_VoxelTerrain_Public_VoxelTerrainComponent_h_57_GENERATED_BODY \
	friend struct ::Z_Construct_UScriptStruct_FVoxelVegetationConfig_Statics; \
	UE_NODEBUG static UScriptStruct* StaticStruct() { return Z_Construct_UScriptStruct_FVoxelVegetationConfig(ETypeConstructPhase::Inner); }


struct FVoxelVegetationConfig;
// ********** End ScriptStruct FVoxelVegetationConfig **********************************************

// ********** Begin Class UVoxelTerrainComponent ***************************************************
#define FID_Users_Administrator_Documents_Unreal_Projects_World_Plugins_VoxelTerrain_Source_VoxelTerrain_Public_VoxelTerrainComponent_h_86_RPC_WRAPPERS_NO_PURE_DECLS \
	DECLARE_FUNCTION(execRequestEdit); \
	DECLARE_FUNCTION(execApplyEdit);


struct Z_Construct_UClass_UVoxelTerrainComponent_Statics;
VOXELTERRAIN_API UClass* Z_Construct_UClass_UVoxelTerrainComponent(ETypeConstructPhase);

#define FID_Users_Administrator_Documents_Unreal_Projects_World_Plugins_VoxelTerrain_Source_VoxelTerrain_Public_VoxelTerrainComponent_h_86_INCLASS_NO_PURE_DECLS \
private: \
	friend struct ::Z_Construct_UClass_UVoxelTerrainComponent_Statics; \
	friend VOXELTERRAIN_API UClass* ::Z_Construct_UClass_UVoxelTerrainComponent(ETypeConstructPhase); \
public: \
	DECLARE_CLASS2(UVoxelTerrainComponent, UPrimitiveComponent, COMPILED_IN_FLAGS(0 | CLASS_Config), CASTCLASS_None, TEXT("/Script/VoxelTerrain"), Z_Construct_UClass_UVoxelTerrainComponent) \
	DECLARE_SERIALIZER(UVoxelTerrainComponent)


#define FID_Users_Administrator_Documents_Unreal_Projects_World_Plugins_VoxelTerrain_Source_VoxelTerrain_Public_VoxelTerrainComponent_h_86_ENHANCED_CONSTRUCTORS \
	/** Deleted move- and copy-constructors, should never be used */ \
	UVoxelTerrainComponent(UVoxelTerrainComponent&&) = delete; \
	UVoxelTerrainComponent(const UVoxelTerrainComponent&) = delete; \
	DECLARE_VTABLE_PTR_HELPER_CTOR(NO_API, UVoxelTerrainComponent); \
	DEFINE_VTABLE_PTR_HELPER_CTOR_CALLER(UVoxelTerrainComponent); \
	DEFINE_DEFAULT_CONSTRUCTOR_CALL(UVoxelTerrainComponent) \
	NO_API virtual ~UVoxelTerrainComponent();


#define FID_Users_Administrator_Documents_Unreal_Projects_World_Plugins_VoxelTerrain_Source_VoxelTerrain_Public_VoxelTerrainComponent_h_83_PROLOG
#define FID_Users_Administrator_Documents_Unreal_Projects_World_Plugins_VoxelTerrain_Source_VoxelTerrain_Public_VoxelTerrainComponent_h_86_GENERATED_BODY \
PRAGMA_DISABLE_DEPRECATION_WARNINGS \
public: \
	FID_Users_Administrator_Documents_Unreal_Projects_World_Plugins_VoxelTerrain_Source_VoxelTerrain_Public_VoxelTerrainComponent_h_86_RPC_WRAPPERS_NO_PURE_DECLS \
	FID_Users_Administrator_Documents_Unreal_Projects_World_Plugins_VoxelTerrain_Source_VoxelTerrain_Public_VoxelTerrainComponent_h_86_INCLASS_NO_PURE_DECLS \
	FID_Users_Administrator_Documents_Unreal_Projects_World_Plugins_VoxelTerrain_Source_VoxelTerrain_Public_VoxelTerrainComponent_h_86_ENHANCED_CONSTRUCTORS \
private: \
PRAGMA_ENABLE_DEPRECATION_WARNINGS


class UVoxelTerrainComponent;

// ********** End Class UVoxelTerrainComponent *****************************************************

#undef CURRENT_FILE_ID
#define CURRENT_FILE_ID FID_Users_Administrator_Documents_Unreal_Projects_World_Plugins_VoxelTerrain_Source_VoxelTerrain_Public_VoxelTerrainComponent_h

PRAGMA_ENABLE_DEPRECATION_WARNINGS
