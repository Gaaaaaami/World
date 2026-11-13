// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "VoxelTerrainComponent.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
static_assert(!UE_WITH_CONSTINIT_UOBJECT, "This generated code can only be compiled with !UE_WITH_CONSTINIT_UOBJECT");
void EmptyLinkFunctionForGeneratedCodeVoxelTerrainComponent() {}

// ********** Begin Cross Module References ********************************************************
COREUOBJECT_API UScriptStruct* Z_Construct_UScriptStruct_FIntVector(ETypeConstructPhase);
COREUOBJECT_API UScriptStruct* Z_Construct_UScriptStruct_FVector(ETypeConstructPhase);
COREUOBJECT_API UScriptStruct* Z_Construct_UScriptStruct_FVector2D(ETypeConstructPhase);
ENGINE_API UClass* Z_Construct_UClass_UPrimitiveComponent(ETypeConstructPhase);
ENGINE_API UClass* Z_Construct_UClass_UHierarchicalInstancedStaticMeshComponent(ETypeConstructPhase);
ENGINE_API UClass* Z_Construct_UClass_UMaterialInterface(ETypeConstructPhase);
ENGINE_API UClass* Z_Construct_UClass_UStaticMesh(ETypeConstructPhase);
ENGINE_API UClass* Z_Construct_UClass_UTexture2D(ETypeConstructPhase);
ENGINE_API UClass* Z_Construct_UClass_UVolumeTexture(ETypeConstructPhase);
PROCEDURALMESHCOMPONENT_API UClass* Z_Construct_UClass_UProceduralMeshComponent(ETypeConstructPhase);
// ********** End Cross Module References **********************************************************

// ********** Begin Same Module References *********************************************************
UPackage* Z_Construct_UPackage__Script_VoxelTerrain(ETypeConstructPhase);
VOXELTERRAIN_API UScriptStruct* Z_Construct_UScriptStruct_FVoxelNoiseLayer3DInfo(ETypeConstructPhase);
VOXELTERRAIN_API UScriptStruct* Z_Construct_UScriptStruct_FVoxelNoiseLayerInfo(ETypeConstructPhase);
VOXELTERRAIN_API UClass* Z_Construct_UClass_UVoxelTerrainComponent(ETypeConstructPhase);
VOXELTERRAIN_API UScriptStruct* Z_Construct_UScriptStruct_FVoxelVegetationConfig(ETypeConstructPhase);
VOXELTERRAIN_API UClass* Z_Construct_UClass_UVoxelTerrainComponent(ETypeConstructPhase);
// ********** End Same Module References ***********************************************************
#define UHT_STRUCT_BASE(INIT) UE::CodeGen::ConstInit::TCompiledInObjectPtr<const FStructBaseChain>(UE::Private::AsStructBaseChain(INIT))

// ********** Begin ScriptStruct FVoxelNoiseLayerInfo **********************************************
#ifdef UHT_STATICS
#error UHT_STATICS already defined
#endif
#define UHT_STATICS Z_Construct_UScriptStruct_FVoxelNoiseLayerInfo_Statics
struct UHT_STATICS
{
	static inline consteval int32 GetStructSize() { return DataSizeOf<FVoxelNoiseLayerInfo>(); }
	static inline consteval int16 GetStructAlignment() { return alignof(FVoxelNoiseLayerInfo); }
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Type_MetaData[] = {
		{ "BlueprintType", "true" },
		{ "ModuleRelativePath", "Public/VoxelTerrainComponent.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_Texture_MetaData[] = {
		{ "Category", "Noise" },
		{ "ModuleRelativePath", "Public/VoxelTerrainComponent.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_Frequency_MetaData[] = {
		{ "Category", "Noise" },
		{ "ModuleRelativePath", "Public/VoxelTerrainComponent.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_Amplitude_MetaData[] = {
		{ "Category", "Noise" },
		{ "ModuleRelativePath", "Public/VoxelTerrainComponent.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_Offset_MetaData[] = {
		{ "Category", "Noise" },
		{ "ModuleRelativePath", "Public/VoxelTerrainComponent.h" },
	};
#endif // WITH_METADATA

// ********** Begin ScriptStruct FVoxelNoiseLayerInfo constinit property declarations **************
	static const UECodeGen_Private::FObjectPropertyParams NewProp_Texture;
	static const UECodeGen_Private::FFloatPropertyParams NewProp_Frequency;
	static const UECodeGen_Private::FFloatPropertyParams NewProp_Amplitude;
	static const UECodeGen_Private::FStructPropertyParams NewProp_Offset;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
// ********** End ScriptStruct FVoxelNoiseLayerInfo constinit property declarations ****************
	static void* NewStructOps()
	{
		return (UScriptStruct::ICppStructOps*)new UScriptStruct::TCppStructOps<FVoxelNoiseLayerInfo>();
	}
	static const UECodeGen_Private::FStructParams StructParams;
}; // struct UHT_STATICS

// ********** Begin ScriptStruct FVoxelNoiseLayerInfo Property Definitions *************************
const UECodeGen_Private::FObjectPropertyParams UHT_STATICS::NewProp_Texture = { "Texture", nullptr, (EPropertyFlags)0x0114000000000001, UECodeGen_Private::EPropertyGenFlags::Object | UECodeGen_Private::EPropertyGenFlags::ObjectPtr, nullptr, nullptr, 1, STRUCT_OFFSET(FVoxelNoiseLayerInfo, Texture), Z_Construct_UClass_UTexture2D, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_Texture_MetaData), NewProp_Texture_MetaData) };
const UECodeGen_Private::FFloatPropertyParams UHT_STATICS::NewProp_Frequency = { "Frequency", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Float, nullptr, nullptr, 1, STRUCT_OFFSET(FVoxelNoiseLayerInfo, Frequency), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_Frequency_MetaData), NewProp_Frequency_MetaData) };
const UECodeGen_Private::FFloatPropertyParams UHT_STATICS::NewProp_Amplitude = { "Amplitude", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Float, nullptr, nullptr, 1, STRUCT_OFFSET(FVoxelNoiseLayerInfo, Amplitude), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_Amplitude_MetaData), NewProp_Amplitude_MetaData) };
const UECodeGen_Private::FStructPropertyParams UHT_STATICS::NewProp_Offset = { "Offset", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Struct, nullptr, nullptr, 1, STRUCT_OFFSET(FVoxelNoiseLayerInfo, Offset), Z_Construct_UScriptStruct_FVector2D, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_Offset_MetaData), NewProp_Offset_MetaData) };
const UECodeGen_Private::FPropertyParamsBase* const UHT_STATICS::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_Texture,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_Frequency,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_Amplitude,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_Offset,
};
static_assert(UE_ARRAY_COUNT(UHT_STATICS::PropPointers) < 2048);
// ********** End ScriptStruct FVoxelNoiseLayerInfo Property Definitions ***************************
const UECodeGen_Private::FStructParams UHT_STATICS::StructParams = {
	(FTypeConstructFunc*)Z_Construct_UPackage__Script_VoxelTerrain,
	nullptr,
	&NewStructOps,
	"VoxelNoiseLayerInfo",
	UHT_STATICS::PropPointers,
	UE_ARRAY_COUNT(UHT_STATICS::PropPointers),
	DataSizeOf<FVoxelNoiseLayerInfo>(),
	alignof(FVoxelNoiseLayerInfo),
	RF_Public|RF_Transient|RF_MarkAsNative,
	EStructFlags(0x00000001),
	METADATA_PARAMS(UE_ARRAY_COUNT(UHT_STATICS::Type_MetaData), UHT_STATICS::Type_MetaData)
};
static FStructRegistrationInfo Z_Registration_Info_UScriptStruct_FVoxelNoiseLayerInfo;
UScriptStruct* Z_Construct_UScriptStruct_FVoxelNoiseLayerInfo(ETypeConstructPhase Phase)
{
	if (Phase == ETypeConstructPhase::Outer)
	{
		if (!Z_Registration_Info_UScriptStruct_FVoxelNoiseLayerInfo.OuterSingleton)
		{
			Z_Registration_Info_UScriptStruct_FVoxelNoiseLayerInfo.OuterSingleton = GetStaticStruct(Z_Construct_UScriptStruct_FVoxelNoiseLayerInfo, (UObject*)Z_Construct_UPackage__Script_VoxelTerrain(ETypeConstructPhase::Outer), TEXT("VoxelNoiseLayerInfo"));
		}
		return Z_Registration_Info_UScriptStruct_FVoxelNoiseLayerInfo.OuterSingleton;
	}
	if (!Z_Registration_Info_UScriptStruct_FVoxelNoiseLayerInfo.InnerSingleton)
	{
		UECodeGen_Private::ConstructUScriptStruct(Z_Registration_Info_UScriptStruct_FVoxelNoiseLayerInfo.InnerSingleton, UHT_STATICS::StructParams);
	}
	return CastChecked<UScriptStruct>(Z_Registration_Info_UScriptStruct_FVoxelNoiseLayerInfo.InnerSingleton);
}
#undef UHT_STATICS
// ********** End ScriptStruct FVoxelNoiseLayerInfo ************************************************

// ********** Begin ScriptStruct FVoxelNoiseLayer3DInfo ********************************************
#ifdef UHT_STATICS
#error UHT_STATICS already defined
#endif
#define UHT_STATICS Z_Construct_UScriptStruct_FVoxelNoiseLayer3DInfo_Statics
struct UHT_STATICS
{
	static inline consteval int32 GetStructSize() { return DataSizeOf<FVoxelNoiseLayer3DInfo>(); }
	static inline consteval int16 GetStructAlignment() { return alignof(FVoxelNoiseLayer3DInfo); }
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Type_MetaData[] = {
		{ "BlueprintType", "true" },
		{ "ModuleRelativePath", "Public/VoxelTerrainComponent.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_Texture_MetaData[] = {
		{ "Category", "Noise" },
		{ "ModuleRelativePath", "Public/VoxelTerrainComponent.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_Frequency_MetaData[] = {
		{ "Category", "Noise" },
		{ "ModuleRelativePath", "Public/VoxelTerrainComponent.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_Amplitude_MetaData[] = {
		{ "Category", "Noise" },
		{ "ModuleRelativePath", "Public/VoxelTerrainComponent.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_Offset_MetaData[] = {
		{ "Category", "Noise" },
		{ "ModuleRelativePath", "Public/VoxelTerrainComponent.h" },
	};
#endif // WITH_METADATA

// ********** Begin ScriptStruct FVoxelNoiseLayer3DInfo constinit property declarations ************
	static const UECodeGen_Private::FObjectPropertyParams NewProp_Texture;
	static const UECodeGen_Private::FFloatPropertyParams NewProp_Frequency;
	static const UECodeGen_Private::FFloatPropertyParams NewProp_Amplitude;
	static const UECodeGen_Private::FStructPropertyParams NewProp_Offset;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
// ********** End ScriptStruct FVoxelNoiseLayer3DInfo constinit property declarations **************
	static void* NewStructOps()
	{
		return (UScriptStruct::ICppStructOps*)new UScriptStruct::TCppStructOps<FVoxelNoiseLayer3DInfo>();
	}
	static const UECodeGen_Private::FStructParams StructParams;
}; // struct UHT_STATICS

// ********** Begin ScriptStruct FVoxelNoiseLayer3DInfo Property Definitions ***********************
const UECodeGen_Private::FObjectPropertyParams UHT_STATICS::NewProp_Texture = { "Texture", nullptr, (EPropertyFlags)0x0114000000000001, UECodeGen_Private::EPropertyGenFlags::Object | UECodeGen_Private::EPropertyGenFlags::ObjectPtr, nullptr, nullptr, 1, STRUCT_OFFSET(FVoxelNoiseLayer3DInfo, Texture), Z_Construct_UClass_UVolumeTexture, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_Texture_MetaData), NewProp_Texture_MetaData) };
const UECodeGen_Private::FFloatPropertyParams UHT_STATICS::NewProp_Frequency = { "Frequency", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Float, nullptr, nullptr, 1, STRUCT_OFFSET(FVoxelNoiseLayer3DInfo, Frequency), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_Frequency_MetaData), NewProp_Frequency_MetaData) };
const UECodeGen_Private::FFloatPropertyParams UHT_STATICS::NewProp_Amplitude = { "Amplitude", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Float, nullptr, nullptr, 1, STRUCT_OFFSET(FVoxelNoiseLayer3DInfo, Amplitude), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_Amplitude_MetaData), NewProp_Amplitude_MetaData) };
const UECodeGen_Private::FStructPropertyParams UHT_STATICS::NewProp_Offset = { "Offset", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Struct, nullptr, nullptr, 1, STRUCT_OFFSET(FVoxelNoiseLayer3DInfo, Offset), Z_Construct_UScriptStruct_FVector, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_Offset_MetaData), NewProp_Offset_MetaData) };
const UECodeGen_Private::FPropertyParamsBase* const UHT_STATICS::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_Texture,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_Frequency,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_Amplitude,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_Offset,
};
static_assert(UE_ARRAY_COUNT(UHT_STATICS::PropPointers) < 2048);
// ********** End ScriptStruct FVoxelNoiseLayer3DInfo Property Definitions *************************
const UECodeGen_Private::FStructParams UHT_STATICS::StructParams = {
	(FTypeConstructFunc*)Z_Construct_UPackage__Script_VoxelTerrain,
	nullptr,
	&NewStructOps,
	"VoxelNoiseLayer3DInfo",
	UHT_STATICS::PropPointers,
	UE_ARRAY_COUNT(UHT_STATICS::PropPointers),
	DataSizeOf<FVoxelNoiseLayer3DInfo>(),
	alignof(FVoxelNoiseLayer3DInfo),
	RF_Public|RF_Transient|RF_MarkAsNative,
	EStructFlags(0x00000001),
	METADATA_PARAMS(UE_ARRAY_COUNT(UHT_STATICS::Type_MetaData), UHT_STATICS::Type_MetaData)
};
static FStructRegistrationInfo Z_Registration_Info_UScriptStruct_FVoxelNoiseLayer3DInfo;
UScriptStruct* Z_Construct_UScriptStruct_FVoxelNoiseLayer3DInfo(ETypeConstructPhase Phase)
{
	if (Phase == ETypeConstructPhase::Outer)
	{
		if (!Z_Registration_Info_UScriptStruct_FVoxelNoiseLayer3DInfo.OuterSingleton)
		{
			Z_Registration_Info_UScriptStruct_FVoxelNoiseLayer3DInfo.OuterSingleton = GetStaticStruct(Z_Construct_UScriptStruct_FVoxelNoiseLayer3DInfo, (UObject*)Z_Construct_UPackage__Script_VoxelTerrain(ETypeConstructPhase::Outer), TEXT("VoxelNoiseLayer3DInfo"));
		}
		return Z_Registration_Info_UScriptStruct_FVoxelNoiseLayer3DInfo.OuterSingleton;
	}
	if (!Z_Registration_Info_UScriptStruct_FVoxelNoiseLayer3DInfo.InnerSingleton)
	{
		UECodeGen_Private::ConstructUScriptStruct(Z_Registration_Info_UScriptStruct_FVoxelNoiseLayer3DInfo.InnerSingleton, UHT_STATICS::StructParams);
	}
	return CastChecked<UScriptStruct>(Z_Registration_Info_UScriptStruct_FVoxelNoiseLayer3DInfo.InnerSingleton);
}
#undef UHT_STATICS
// ********** End ScriptStruct FVoxelNoiseLayer3DInfo **********************************************

// ********** Begin ScriptStruct FVoxelVegetationConfig ********************************************
#ifdef UHT_STATICS
#error UHT_STATICS already defined
#endif
#define UHT_STATICS Z_Construct_UScriptStruct_FVoxelVegetationConfig_Statics
struct UHT_STATICS
{
	static inline consteval int32 GetStructSize() { return DataSizeOf<FVoxelVegetationConfig>(); }
	static inline consteval int16 GetStructAlignment() { return alignof(FVoxelVegetationConfig); }
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Type_MetaData[] = {
		{ "BlueprintType", "true" },
		{ "ModuleRelativePath", "Public/VoxelTerrainComponent.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_MinID_MetaData[] = {
		{ "Category", "Vegetation" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "// \xe8\xbf\x99\xe4\xb8\xaa\xe6\xa8\xa1\xe5\x9e\x8b\xe5\x9c\xa8\xe5\x93\xaa\xe4\xb8\xaa\xe5\x99\xaa\xe7\x82\xb9\xe5\x8c\xba\xe9\x97\xb4\xe7\x94\x9f\xe6\x88\x90\xef\xbc\x88\xe6\x94\xaf\xe6\x8c\x81\xe5\xa4\x9a\xe4\xb8\xaa\xe6\xa8\xa1\xe5\x9e\x8b\xe5\xa1\xab\xe5\x90\x8c\xe6\xa0\xb7\xe7\x9a\x84\xe5\x8c\xba\xe9\x97\xb4\xe6\x9d\xa5\xe5\xae\x9e\xe7\x8e\xb0\xe6\xb7\xb7\xe5\x90\x88\xef\xbc\x89\n" },
#endif
		{ "ModuleRelativePath", "Public/VoxelTerrainComponent.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "\xe8\xbf\x99\xe4\xb8\xaa\xe6\xa8\xa1\xe5\x9e\x8b\xe5\x9c\xa8\xe5\x93\xaa\xe4\xb8\xaa\xe5\x99\xaa\xe7\x82\xb9\xe5\x8c\xba\xe9\x97\xb4\xe7\x94\x9f\xe6\x88\x90\xef\xbc\x88\xe6\x94\xaf\xe6\x8c\x81\xe5\xa4\x9a\xe4\xb8\xaa\xe6\xa8\xa1\xe5\x9e\x8b\xe5\xa1\xab\xe5\x90\x8c\xe6\xa0\xb7\xe7\x9a\x84\xe5\x8c\xba\xe9\x97\xb4\xe6\x9d\xa5\xe5\xae\x9e\xe7\x8e\xb0\xe6\xb7\xb7\xe5\x90\x88\xef\xbc\x89" },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_MaxID_MetaData[] = {
		{ "Category", "Vegetation" },
		{ "ModuleRelativePath", "Public/VoxelTerrainComponent.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_Mesh_MetaData[] = {
		{ "Category", "Vegetation" },
		{ "ModuleRelativePath", "Public/VoxelTerrainComponent.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_Density_MetaData[] = {
		{ "Category", "Vegetation" },
		{ "ModuleRelativePath", "Public/VoxelTerrainComponent.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_PlaceRadius_MetaData[] = {
		{ "Category", "Vegetation" },
		{ "ModuleRelativePath", "Public/VoxelTerrainComponent.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_MinScale_MetaData[] = {
		{ "Category", "Vegetation" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "// \xe9\x98\xb2\xe9\x87\x8d\xe5\x8f\xa0\xe5\x8d\x8a\xe5\xbe\x84\n" },
#endif
		{ "ModuleRelativePath", "Public/VoxelTerrainComponent.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "\xe9\x98\xb2\xe9\x87\x8d\xe5\x8f\xa0\xe5\x8d\x8a\xe5\xbe\x84" },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_MaxScale_MetaData[] = {
		{ "Category", "Vegetation" },
		{ "ModuleRelativePath", "Public/VoxelTerrainComponent.h" },
	};
#endif // WITH_METADATA

// ********** Begin ScriptStruct FVoxelVegetationConfig constinit property declarations ************
	static const UECodeGen_Private::FFloatPropertyParams NewProp_MinID;
	static const UECodeGen_Private::FFloatPropertyParams NewProp_MaxID;
	static const UECodeGen_Private::FObjectPropertyParams NewProp_Mesh;
	static const UECodeGen_Private::FFloatPropertyParams NewProp_Density;
	static const UECodeGen_Private::FFloatPropertyParams NewProp_PlaceRadius;
	static const UECodeGen_Private::FFloatPropertyParams NewProp_MinScale;
	static const UECodeGen_Private::FFloatPropertyParams NewProp_MaxScale;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
// ********** End ScriptStruct FVoxelVegetationConfig constinit property declarations **************
	static void* NewStructOps()
	{
		return (UScriptStruct::ICppStructOps*)new UScriptStruct::TCppStructOps<FVoxelVegetationConfig>();
	}
	static const UECodeGen_Private::FStructParams StructParams;
}; // struct UHT_STATICS

// ********** Begin ScriptStruct FVoxelVegetationConfig Property Definitions ***********************
const UECodeGen_Private::FFloatPropertyParams UHT_STATICS::NewProp_MinID = { "MinID", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Float, nullptr, nullptr, 1, STRUCT_OFFSET(FVoxelVegetationConfig, MinID), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_MinID_MetaData), NewProp_MinID_MetaData) };
const UECodeGen_Private::FFloatPropertyParams UHT_STATICS::NewProp_MaxID = { "MaxID", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Float, nullptr, nullptr, 1, STRUCT_OFFSET(FVoxelVegetationConfig, MaxID), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_MaxID_MetaData), NewProp_MaxID_MetaData) };
const UECodeGen_Private::FObjectPropertyParams UHT_STATICS::NewProp_Mesh = { "Mesh", nullptr, (EPropertyFlags)0x0114000000000001, UECodeGen_Private::EPropertyGenFlags::Object | UECodeGen_Private::EPropertyGenFlags::ObjectPtr, nullptr, nullptr, 1, STRUCT_OFFSET(FVoxelVegetationConfig, Mesh), Z_Construct_UClass_UStaticMesh, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_Mesh_MetaData), NewProp_Mesh_MetaData) };
const UECodeGen_Private::FFloatPropertyParams UHT_STATICS::NewProp_Density = { "Density", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Float, nullptr, nullptr, 1, STRUCT_OFFSET(FVoxelVegetationConfig, Density), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_Density_MetaData), NewProp_Density_MetaData) };
const UECodeGen_Private::FFloatPropertyParams UHT_STATICS::NewProp_PlaceRadius = { "PlaceRadius", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Float, nullptr, nullptr, 1, STRUCT_OFFSET(FVoxelVegetationConfig, PlaceRadius), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_PlaceRadius_MetaData), NewProp_PlaceRadius_MetaData) };
const UECodeGen_Private::FFloatPropertyParams UHT_STATICS::NewProp_MinScale = { "MinScale", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Float, nullptr, nullptr, 1, STRUCT_OFFSET(FVoxelVegetationConfig, MinScale), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_MinScale_MetaData), NewProp_MinScale_MetaData) };
const UECodeGen_Private::FFloatPropertyParams UHT_STATICS::NewProp_MaxScale = { "MaxScale", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Float, nullptr, nullptr, 1, STRUCT_OFFSET(FVoxelVegetationConfig, MaxScale), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_MaxScale_MetaData), NewProp_MaxScale_MetaData) };
const UECodeGen_Private::FPropertyParamsBase* const UHT_STATICS::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_MinID,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_MaxID,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_Mesh,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_Density,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_PlaceRadius,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_MinScale,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_MaxScale,
};
static_assert(UE_ARRAY_COUNT(UHT_STATICS::PropPointers) < 2048);
// ********** End ScriptStruct FVoxelVegetationConfig Property Definitions *************************
const UECodeGen_Private::FStructParams UHT_STATICS::StructParams = {
	(FTypeConstructFunc*)Z_Construct_UPackage__Script_VoxelTerrain,
	nullptr,
	&NewStructOps,
	"VoxelVegetationConfig",
	UHT_STATICS::PropPointers,
	UE_ARRAY_COUNT(UHT_STATICS::PropPointers),
	DataSizeOf<FVoxelVegetationConfig>(),
	alignof(FVoxelVegetationConfig),
	RF_Public|RF_Transient|RF_MarkAsNative,
	EStructFlags(0x00000001),
	METADATA_PARAMS(UE_ARRAY_COUNT(UHT_STATICS::Type_MetaData), UHT_STATICS::Type_MetaData)
};
static FStructRegistrationInfo Z_Registration_Info_UScriptStruct_FVoxelVegetationConfig;
UScriptStruct* Z_Construct_UScriptStruct_FVoxelVegetationConfig(ETypeConstructPhase Phase)
{
	if (Phase == ETypeConstructPhase::Outer)
	{
		if (!Z_Registration_Info_UScriptStruct_FVoxelVegetationConfig.OuterSingleton)
		{
			Z_Registration_Info_UScriptStruct_FVoxelVegetationConfig.OuterSingleton = GetStaticStruct(Z_Construct_UScriptStruct_FVoxelVegetationConfig, (UObject*)Z_Construct_UPackage__Script_VoxelTerrain(ETypeConstructPhase::Outer), TEXT("VoxelVegetationConfig"));
		}
		return Z_Registration_Info_UScriptStruct_FVoxelVegetationConfig.OuterSingleton;
	}
	if (!Z_Registration_Info_UScriptStruct_FVoxelVegetationConfig.InnerSingleton)
	{
		UECodeGen_Private::ConstructUScriptStruct(Z_Registration_Info_UScriptStruct_FVoxelVegetationConfig.InnerSingleton, UHT_STATICS::StructParams);
	}
	return CastChecked<UScriptStruct>(Z_Registration_Info_UScriptStruct_FVoxelVegetationConfig.InnerSingleton);
}
#undef UHT_STATICS
// ********** End ScriptStruct FVoxelVegetationConfig **********************************************

// ********** Begin Class UVoxelTerrainComponent Function ApplyEdit ********************************
#ifdef UHT_STATICS
#error UHT_STATICS already defined
#endif
#define UHT_STATICS Z_Construct_UFunction_UVoxelTerrainComponent_ApplyEdit_Statics
struct UHT_STATICS
{
	struct VoxelTerrainComponent_eventApplyEdit_Parms
	{
		FVector WorldPos;
		float Radius;
		bool bIsAdd;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Type_MetaData[] = {
		{ "Category", "Voxel" },
		{ "ModuleRelativePath", "Public/VoxelTerrainComponent.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_WorldPos_MetaData[] = {
		{ "NativeConst", "" },
	};
#endif // WITH_METADATA

// ********** Begin Function ApplyEdit constinit property declarations *****************************
	static const UECodeGen_Private::FStructPropertyParams NewProp_WorldPos;
	static const UECodeGen_Private::FFloatPropertyParams NewProp_Radius;
	static void NewProp_bIsAdd_SetBit(void* Obj)
	{
		((VoxelTerrainComponent_eventApplyEdit_Parms*)Obj)->bIsAdd = 1;
	}
	static const UECodeGen_Private::FBoolPropertyParams NewProp_bIsAdd;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
// ********** End Function ApplyEdit constinit property declarations *******************************
	static const UECodeGen_Private::FFunctionParams FuncParams;
};

// ********** Begin Function ApplyEdit Property Definitions ****************************************
const UECodeGen_Private::FStructPropertyParams UHT_STATICS::NewProp_WorldPos = { "WorldPos", nullptr, (EPropertyFlags)0x0010000008000182, UECodeGen_Private::EPropertyGenFlags::Struct, nullptr, nullptr, 1, STRUCT_OFFSET(VoxelTerrainComponent_eventApplyEdit_Parms, WorldPos), Z_Construct_UScriptStruct_FVector, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_WorldPos_MetaData), NewProp_WorldPos_MetaData) };
const UECodeGen_Private::FFloatPropertyParams UHT_STATICS::NewProp_Radius = { "Radius", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Float, nullptr, nullptr, 1, STRUCT_OFFSET(VoxelTerrainComponent_eventApplyEdit_Parms, Radius), METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FBoolPropertyParams UHT_STATICS::NewProp_bIsAdd = { "bIsAdd", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Bool | UECodeGen_Private::EPropertyGenFlags::NativeBool, nullptr, nullptr, 1, sizeof(bool), sizeof(VoxelTerrainComponent_eventApplyEdit_Parms), &UHT_STATICS::NewProp_bIsAdd_SetBit, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FPropertyParamsBase* const UHT_STATICS::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_WorldPos,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_Radius,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_bIsAdd,
};
static_assert(UE_ARRAY_COUNT(UHT_STATICS::PropPointers) < 2048);
// ********** End Function ApplyEdit Property Definitions ******************************************
const UECodeGen_Private::FFunctionParams UHT_STATICS::FuncParams = { { (FTypeConstructFunc*)Z_Construct_UClass_UVoxelTerrainComponent, nullptr, "ApplyEdit", UHT_STATICS::PropPointers, UE_ARRAY_COUNT(UHT_STATICS::PropPointers), DataSizeOf<UHT_STATICS::VoxelTerrainComponent_eventApplyEdit_Parms>(), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x04C20401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(UHT_STATICS::Type_MetaData), UHT_STATICS::Type_MetaData)},  };
static_assert(sizeof(UHT_STATICS::VoxelTerrainComponent_eventApplyEdit_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_UVoxelTerrainComponent_ApplyEdit(ETypeConstructPhase Phase)
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, UHT_STATICS::FuncParams);
	}
	return ReturnFunction;
}
#undef UHT_STATICS
DEFINE_FUNCTION(UVoxelTerrainComponent::execApplyEdit)
{
	P_GET_STRUCT_REF(FVector,Z_Param_Out_WorldPos);
	P_GET_PROPERTY(FFloatProperty,Z_Param_Radius);
	P_GET_UBOOL(Z_Param_bIsAdd);
	P_FINISH;
	P_NATIVE_BEGIN;
	P_THIS->ApplyEdit(Z_Param_Out_WorldPos,Z_Param_Radius,Z_Param_bIsAdd);
	P_NATIVE_END;
}
// ********** End Class UVoxelTerrainComponent Function ApplyEdit **********************************

// ********** Begin Class UVoxelTerrainComponent Function RequestEdit ******************************
#ifdef UHT_STATICS
#error UHT_STATICS already defined
#endif
#define UHT_STATICS Z_Construct_UFunction_UVoxelTerrainComponent_RequestEdit_Statics
struct UHT_STATICS
{
	struct VoxelTerrainComponent_eventRequestEdit_Parms
	{
		FVector WorldPos;
		float Radius;
		bool bIsAdd;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Type_MetaData[] = {
		{ "Category", "Voxel" },
		{ "ModuleRelativePath", "Public/VoxelTerrainComponent.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_WorldPos_MetaData[] = {
		{ "NativeConst", "" },
	};
#endif // WITH_METADATA

// ********** Begin Function RequestEdit constinit property declarations ***************************
	static const UECodeGen_Private::FStructPropertyParams NewProp_WorldPos;
	static const UECodeGen_Private::FFloatPropertyParams NewProp_Radius;
	static void NewProp_bIsAdd_SetBit(void* Obj)
	{
		((VoxelTerrainComponent_eventRequestEdit_Parms*)Obj)->bIsAdd = 1;
	}
	static const UECodeGen_Private::FBoolPropertyParams NewProp_bIsAdd;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
// ********** End Function RequestEdit constinit property declarations *****************************
	static const UECodeGen_Private::FFunctionParams FuncParams;
};

// ********** Begin Function RequestEdit Property Definitions **************************************
const UECodeGen_Private::FStructPropertyParams UHT_STATICS::NewProp_WorldPos = { "WorldPos", nullptr, (EPropertyFlags)0x0010000008000182, UECodeGen_Private::EPropertyGenFlags::Struct, nullptr, nullptr, 1, STRUCT_OFFSET(VoxelTerrainComponent_eventRequestEdit_Parms, WorldPos), Z_Construct_UScriptStruct_FVector, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_WorldPos_MetaData), NewProp_WorldPos_MetaData) };
const UECodeGen_Private::FFloatPropertyParams UHT_STATICS::NewProp_Radius = { "Radius", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Float, nullptr, nullptr, 1, STRUCT_OFFSET(VoxelTerrainComponent_eventRequestEdit_Parms, Radius), METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FBoolPropertyParams UHT_STATICS::NewProp_bIsAdd = { "bIsAdd", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Bool | UECodeGen_Private::EPropertyGenFlags::NativeBool, nullptr, nullptr, 1, sizeof(bool), sizeof(VoxelTerrainComponent_eventRequestEdit_Parms), &UHT_STATICS::NewProp_bIsAdd_SetBit, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FPropertyParamsBase* const UHT_STATICS::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_WorldPos,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_Radius,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_bIsAdd,
};
static_assert(UE_ARRAY_COUNT(UHT_STATICS::PropPointers) < 2048);
// ********** End Function RequestEdit Property Definitions ****************************************
const UECodeGen_Private::FFunctionParams UHT_STATICS::FuncParams = { { (FTypeConstructFunc*)Z_Construct_UClass_UVoxelTerrainComponent, nullptr, "RequestEdit", UHT_STATICS::PropPointers, UE_ARRAY_COUNT(UHT_STATICS::PropPointers), DataSizeOf<UHT_STATICS::VoxelTerrainComponent_eventRequestEdit_Parms>(), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x04C20401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(UHT_STATICS::Type_MetaData), UHT_STATICS::Type_MetaData)},  };
static_assert(sizeof(UHT_STATICS::VoxelTerrainComponent_eventRequestEdit_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_UVoxelTerrainComponent_RequestEdit(ETypeConstructPhase Phase)
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, UHT_STATICS::FuncParams);
	}
	return ReturnFunction;
}
#undef UHT_STATICS
DEFINE_FUNCTION(UVoxelTerrainComponent::execRequestEdit)
{
	P_GET_STRUCT_REF(FVector,Z_Param_Out_WorldPos);
	P_GET_PROPERTY(FFloatProperty,Z_Param_Radius);
	P_GET_UBOOL(Z_Param_bIsAdd);
	P_FINISH;
	P_NATIVE_BEGIN;
	P_THIS->RequestEdit(Z_Param_Out_WorldPos,Z_Param_Radius,Z_Param_bIsAdd);
	P_NATIVE_END;
}
// ********** End Class UVoxelTerrainComponent Function RequestEdit ********************************

// ********** Begin Class UVoxelTerrainComponent ***************************************************
#ifdef UHT_STATICS
#error UHT_STATICS already defined
#endif
#define UHT_STATICS Z_Construct_UClass_UVoxelTerrainComponent_Statics
struct UHT_STATICS
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Type_MetaData[] = {
		{ "BlueprintSpawnableComponent", "" },
		{ "ClassGroupNames", "Voxel" },
		{ "HideCategories", "Mobility VirtualTexture Trigger" },
		{ "IncludePath", "VoxelTerrainComponent.h" },
		{ "ModuleRelativePath", "Public/VoxelTerrainComponent.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_TerrainMaterial_MetaData[] = {
		{ "Category", "Voxel" },
		{ "ModuleRelativePath", "Public/VoxelTerrainComponent.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_LOD0RadiusChunksXY_MetaData[] = {
		{ "Category", "Voxel|LOD" },
		{ "DisplayName", "LOD0 Radius XY (Chunks)" },
		{ "ModuleRelativePath", "Public/VoxelTerrainComponent.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_LOD0RadiusChunksZ_MetaData[] = {
		{ "Category", "Voxel|LOD" },
		{ "DisplayName", "LOD0 Radius Z (Chunks)" },
		{ "ModuleRelativePath", "Public/VoxelTerrainComponent.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_UnloadDelay_MetaData[] = {
		{ "Category", "Voxel" },
		{ "ModuleRelativePath", "Public/VoxelTerrainComponent.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_bGenerateCollision_MetaData[] = {
		{ "Category", "Voxel" },
		{ "ModuleRelativePath", "Public/VoxelTerrainComponent.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_CollisionRadius_MetaData[] = {
		{ "Category", "Voxel|Collision" },
		{ "ModuleRelativePath", "Public/VoxelTerrainComponent.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_CollisionComponentPool_MetaData[] = {
		{ "EditInline", "true" },
		{ "ModuleRelativePath", "Public/VoxelTerrainComponent.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_NoiseLayers_MetaData[] = {
		{ "Category", "Voxel|Noise" },
		{ "ModuleRelativePath", "Public/VoxelTerrainComponent.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_NoiseLayers3D_MetaData[] = {
		{ "Category", "Voxel|Noise3D" },
		{ "ModuleRelativePath", "Public/VoxelTerrainComponent.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_BiomeTexture_MetaData[] = {
		{ "Category", "Voxel|Vegetation" },
		{ "ModuleRelativePath", "Public/VoxelTerrainComponent.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_VegetationConfigs_MetaData[] = {
		{ "Category", "Voxel|Vegetation" },
		{ "ModuleRelativePath", "Public/VoxelTerrainComponent.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_HISMPool_MetaData[] = {
#if !UE_BUILD_SHIPPING
		{ "Comment", "// \xe2\xad\x90 HISM \xe5\xaf\xb9\xe8\xb1\xa1\xe6\xb1\xa0\n" },
#endif
		{ "EditInline", "true" },
		{ "ModuleRelativePath", "Public/VoxelTerrainComponent.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "\xe2\xad\x90 HISM \xe5\xaf\xb9\xe8\xb1\xa1\xe6\xb1\xa0" },
#endif
	};
#endif // WITH_METADATA

// ********** Begin Class UVoxelTerrainComponent constinit property declarations *******************
	static const UECodeGen_Private::FObjectPropertyParams NewProp_TerrainMaterial;
	static const UECodeGen_Private::FIntPropertyParams NewProp_LOD0RadiusChunksXY;
	static const UECodeGen_Private::FIntPropertyParams NewProp_LOD0RadiusChunksZ;
	static const UECodeGen_Private::FFloatPropertyParams NewProp_UnloadDelay;
	static void NewProp_bGenerateCollision_SetBit(void* Obj)
	{
		((UVoxelTerrainComponent*)Obj)->bGenerateCollision = 1;
	}
	static const UECodeGen_Private::FBoolPropertyParams NewProp_bGenerateCollision;
	static const UECodeGen_Private::FStructPropertyParams NewProp_CollisionRadius;
	static const UECodeGen_Private::FObjectPropertyParams NewProp_CollisionComponentPool_Inner;
	static const UECodeGen_Private::FArrayPropertyParams NewProp_CollisionComponentPool;
	static const UECodeGen_Private::FStructPropertyParams NewProp_NoiseLayers_Inner;
	static const UECodeGen_Private::FArrayPropertyParams NewProp_NoiseLayers;
	static const UECodeGen_Private::FStructPropertyParams NewProp_NoiseLayers3D_Inner;
	static const UECodeGen_Private::FArrayPropertyParams NewProp_NoiseLayers3D;
	static const UECodeGen_Private::FObjectPropertyParams NewProp_BiomeTexture;
	static const UECodeGen_Private::FStructPropertyParams NewProp_VegetationConfigs_Inner;
	static const UECodeGen_Private::FArrayPropertyParams NewProp_VegetationConfigs;
	static const UECodeGen_Private::FObjectPropertyParams NewProp_HISMPool_Inner;
	static const UECodeGen_Private::FArrayPropertyParams NewProp_HISMPool;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
// ********** End Class UVoxelTerrainComponent constinit property declarations *********************
	static constexpr UE::CodeGen::FClassNativeFunction Funcs[] = {
		{ .NameUTF8 = UTF8TEXT("ApplyEdit"), .Pointer = &UVoxelTerrainComponent::execApplyEdit },
		{ .NameUTF8 = UTF8TEXT("RequestEdit"), .Pointer = &UVoxelTerrainComponent::execRequestEdit },
	};
	static FTypeConstructFunc* DependentSingletons[];
	static constexpr FClassFunctionLinkInfo FuncInfo[] = {
		{ &Z_Construct_UFunction_UVoxelTerrainComponent_ApplyEdit, "ApplyEdit" }, // c2436e9c6f20010e7cf1f092b5a7ef761a69d398
		{ &Z_Construct_UFunction_UVoxelTerrainComponent_RequestEdit, "RequestEdit" }, // 78cb82f34fbaf59340d7da5ab2b1b940639d0b84
	};
	static_assert(UE_ARRAY_COUNT(FuncInfo) < 2048);
	static constexpr FCppClassTypeInfoStatic StaticCppClassTypeInfo = {
		TCppClassTypeTraits<UVoxelTerrainComponent>::IsAbstract,
	};
	static const UECodeGen_Private::FClassParams ClassParams;
}; // struct UHT_STATICS

// ********** Begin Class UVoxelTerrainComponent Property Definitions ******************************
const UECodeGen_Private::FObjectPropertyParams UHT_STATICS::NewProp_TerrainMaterial = { "TerrainMaterial", nullptr, (EPropertyFlags)0x0114000000000001, UECodeGen_Private::EPropertyGenFlags::Object | UECodeGen_Private::EPropertyGenFlags::ObjectPtr, nullptr, nullptr, 1, STRUCT_OFFSET(UVoxelTerrainComponent, TerrainMaterial), Z_Construct_UClass_UMaterialInterface, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_TerrainMaterial_MetaData), NewProp_TerrainMaterial_MetaData) };
const UECodeGen_Private::FIntPropertyParams UHT_STATICS::NewProp_LOD0RadiusChunksXY = { "LOD0RadiusChunksXY", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Int, nullptr, nullptr, 1, STRUCT_OFFSET(UVoxelTerrainComponent, LOD0RadiusChunksXY), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_LOD0RadiusChunksXY_MetaData), NewProp_LOD0RadiusChunksXY_MetaData) };
const UECodeGen_Private::FIntPropertyParams UHT_STATICS::NewProp_LOD0RadiusChunksZ = { "LOD0RadiusChunksZ", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Int, nullptr, nullptr, 1, STRUCT_OFFSET(UVoxelTerrainComponent, LOD0RadiusChunksZ), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_LOD0RadiusChunksZ_MetaData), NewProp_LOD0RadiusChunksZ_MetaData) };
const UECodeGen_Private::FFloatPropertyParams UHT_STATICS::NewProp_UnloadDelay = { "UnloadDelay", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Float, nullptr, nullptr, 1, STRUCT_OFFSET(UVoxelTerrainComponent, UnloadDelay), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_UnloadDelay_MetaData), NewProp_UnloadDelay_MetaData) };
const UECodeGen_Private::FBoolPropertyParams UHT_STATICS::NewProp_bGenerateCollision = { "bGenerateCollision", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Bool | UECodeGen_Private::EPropertyGenFlags::NativeBool, nullptr, nullptr, 1, sizeof(bool), sizeof(UVoxelTerrainComponent), &UHT_STATICS::NewProp_bGenerateCollision_SetBit, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_bGenerateCollision_MetaData), NewProp_bGenerateCollision_MetaData) };
const UECodeGen_Private::FStructPropertyParams UHT_STATICS::NewProp_CollisionRadius = { "CollisionRadius", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Struct, nullptr, nullptr, 1, STRUCT_OFFSET(UVoxelTerrainComponent, CollisionRadius), Z_Construct_UScriptStruct_FIntVector, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_CollisionRadius_MetaData), NewProp_CollisionRadius_MetaData) };
const UECodeGen_Private::FObjectPropertyParams UHT_STATICS::NewProp_CollisionComponentPool_Inner = { "CollisionComponentPool", nullptr, (EPropertyFlags)0x0104000000080008, UECodeGen_Private::EPropertyGenFlags::Object | UECodeGen_Private::EPropertyGenFlags::ObjectPtr, nullptr, nullptr, 1, 0, Z_Construct_UClass_UProceduralMeshComponent, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FArrayPropertyParams UHT_STATICS::NewProp_CollisionComponentPool = { "CollisionComponentPool", nullptr, (EPropertyFlags)0x0144008000000008, UECodeGen_Private::EPropertyGenFlags::Array, nullptr, nullptr, 1, STRUCT_OFFSET(UVoxelTerrainComponent, CollisionComponentPool), EArrayPropertyFlags::None, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_CollisionComponentPool_MetaData), NewProp_CollisionComponentPool_MetaData) };
const UECodeGen_Private::FStructPropertyParams UHT_STATICS::NewProp_NoiseLayers_Inner = { "NoiseLayers", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Struct, nullptr, nullptr, 1, 0, Z_Construct_UScriptStruct_FVoxelNoiseLayerInfo, METADATA_PARAMS(0, nullptr) }; // 0623884665795223aacb6bd36db67d0726694754
const UECodeGen_Private::FArrayPropertyParams UHT_STATICS::NewProp_NoiseLayers = { "NoiseLayers", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Array, nullptr, nullptr, 1, STRUCT_OFFSET(UVoxelTerrainComponent, NoiseLayers), EArrayPropertyFlags::None, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_NoiseLayers_MetaData), NewProp_NoiseLayers_MetaData) }; // 0623884665795223aacb6bd36db67d0726694754
const UECodeGen_Private::FStructPropertyParams UHT_STATICS::NewProp_NoiseLayers3D_Inner = { "NoiseLayers3D", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Struct, nullptr, nullptr, 1, 0, Z_Construct_UScriptStruct_FVoxelNoiseLayer3DInfo, METADATA_PARAMS(0, nullptr) }; // 982729d5df116956271d18feee385be6fed872dd
const UECodeGen_Private::FArrayPropertyParams UHT_STATICS::NewProp_NoiseLayers3D = { "NoiseLayers3D", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Array, nullptr, nullptr, 1, STRUCT_OFFSET(UVoxelTerrainComponent, NoiseLayers3D), EArrayPropertyFlags::None, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_NoiseLayers3D_MetaData), NewProp_NoiseLayers3D_MetaData) }; // 982729d5df116956271d18feee385be6fed872dd
const UECodeGen_Private::FObjectPropertyParams UHT_STATICS::NewProp_BiomeTexture = { "BiomeTexture", nullptr, (EPropertyFlags)0x0114000000000001, UECodeGen_Private::EPropertyGenFlags::Object | UECodeGen_Private::EPropertyGenFlags::ObjectPtr, nullptr, nullptr, 1, STRUCT_OFFSET(UVoxelTerrainComponent, BiomeTexture), Z_Construct_UClass_UTexture2D, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_BiomeTexture_MetaData), NewProp_BiomeTexture_MetaData) };
const UECodeGen_Private::FStructPropertyParams UHT_STATICS::NewProp_VegetationConfigs_Inner = { "VegetationConfigs", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Struct, nullptr, nullptr, 1, 0, Z_Construct_UScriptStruct_FVoxelVegetationConfig, METADATA_PARAMS(0, nullptr) }; // 5978ccc49cb8298b0e66512c04fb44dd10004631
const UECodeGen_Private::FArrayPropertyParams UHT_STATICS::NewProp_VegetationConfigs = { "VegetationConfigs", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Array, nullptr, nullptr, 1, STRUCT_OFFSET(UVoxelTerrainComponent, VegetationConfigs), EArrayPropertyFlags::None, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_VegetationConfigs_MetaData), NewProp_VegetationConfigs_MetaData) }; // 5978ccc49cb8298b0e66512c04fb44dd10004631
const UECodeGen_Private::FObjectPropertyParams UHT_STATICS::NewProp_HISMPool_Inner = { "HISMPool", nullptr, (EPropertyFlags)0x0104000000080008, UECodeGen_Private::EPropertyGenFlags::Object | UECodeGen_Private::EPropertyGenFlags::ObjectPtr, nullptr, nullptr, 1, 0, Z_Construct_UClass_UHierarchicalInstancedStaticMeshComponent, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FArrayPropertyParams UHT_STATICS::NewProp_HISMPool = { "HISMPool", nullptr, (EPropertyFlags)0x0144008000000008, UECodeGen_Private::EPropertyGenFlags::Array, nullptr, nullptr, 1, STRUCT_OFFSET(UVoxelTerrainComponent, HISMPool), EArrayPropertyFlags::None, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_HISMPool_MetaData), NewProp_HISMPool_MetaData) };
const UECodeGen_Private::FPropertyParamsBase* const UHT_STATICS::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_TerrainMaterial,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_LOD0RadiusChunksXY,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_LOD0RadiusChunksZ,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_UnloadDelay,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_bGenerateCollision,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_CollisionRadius,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_CollisionComponentPool_Inner,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_CollisionComponentPool,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_NoiseLayers_Inner,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_NoiseLayers,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_NoiseLayers3D_Inner,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_NoiseLayers3D,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_BiomeTexture,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_VegetationConfigs_Inner,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_VegetationConfigs,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_HISMPool_Inner,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_HISMPool,
};
static_assert(UE_ARRAY_COUNT(UHT_STATICS::PropPointers) < 2048);
// ********** End Class UVoxelTerrainComponent Property Definitions ********************************
FTypeConstructFunc* UHT_STATICS::DependentSingletons[] = {
	(FTypeConstructFunc*)Z_Construct_UClass_UPrimitiveComponent,
	(FTypeConstructFunc*)Z_Construct_UPackage__Script_VoxelTerrain,
};
static_assert(UE_ARRAY_COUNT(UHT_STATICS::DependentSingletons) < 16);
const UECodeGen_Private::FClassParams UHT_STATICS::ClassParams = {
	&Z_Construct_UClass_UVoxelTerrainComponent,
	"Engine",
	&StaticCppClassTypeInfo,
	DependentSingletons,
	FuncInfo,
	UHT_STATICS::PropPointers,
	nullptr,
	UE_ARRAY_COUNT(DependentSingletons),
	UE_ARRAY_COUNT(FuncInfo),
	UE_ARRAY_COUNT(UHT_STATICS::PropPointers),
	0,
	0x00B000A4u,
	METADATA_PARAMS(UE_ARRAY_COUNT(UHT_STATICS::Type_MetaData), UHT_STATICS::Type_MetaData)
};
static void UVoxelTerrainComponent_StaticRegisterNativesUVoxelTerrainComponent()
{
	UClass* Class = UVoxelTerrainComponent::StaticClass();
	FNativeFunctionRegistrar::RegisterFunctions(Class, 		MakeConstArrayView(UHT_STATICS::Funcs));
}
FClassRegistrationInfo Z_Registration_Info_UClass_UVoxelTerrainComponent;
UClass* Z_Construct_UClass_UVoxelTerrainComponent(ETypeConstructPhase Phase)
{
	if (Phase == ETypeConstructPhase::Inner)
	{
		using TClass = UVoxelTerrainComponent;
		if (!Z_Registration_Info_UClass_UVoxelTerrainComponent.InnerSingleton)
		{
			GetPrivateStaticClassBody(
				TClass::StaticPackage(),
				TEXT("VoxelTerrainComponent"),
				Z_Registration_Info_UClass_UVoxelTerrainComponent.InnerSingleton,
				UVoxelTerrainComponent_StaticRegisterNativesUVoxelTerrainComponent,
				DataSizeOf<TClass>(),
				alignof(TClass),
				TClass::StaticClassFlags,
				TClass::StaticClassCastFlags(),
				TClass::StaticConfigName(),
				(UClass::ClassConstructorType)InternalConstructor<TClass>,
				(UClass::ClassVTableHelperCtorCallerType)InternalVTableHelperCtorCaller<TClass>,
				UOBJECT_CPPCLASS_STATICFUNCTIONS_FORCLASS(TClass),
				&TClass::Super::StaticClass,
				&TClass::WithinClass::StaticClass
			);
		}
		return Z_Registration_Info_UClass_UVoxelTerrainComponent.InnerSingleton;
	}
	if (!Z_Registration_Info_UClass_UVoxelTerrainComponent.OuterSingleton)
	{
		UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_UVoxelTerrainComponent.OuterSingleton, UHT_STATICS::ClassParams);
	}
	return Z_Registration_Info_UClass_UVoxelTerrainComponent.OuterSingleton;
}
#undef UHT_STATICS
DEFINE_VTABLE_PTR_HELPER_CTOR_NS(, UVoxelTerrainComponent);
UVoxelTerrainComponent::~UVoxelTerrainComponent() {}
// ********** End Class UVoxelTerrainComponent *****************************************************

// ********** Begin Registration *******************************************************************
#ifdef UHT_STATICS
#error UHT_STATICS already defined
#endif
#define UHT_STATICS Z_CompiledInDeferFile_FID_Users_Administrator_Documents_Unreal_Projects_World_Plugins_VoxelTerrain_Source_VoxelTerrain_Public_VoxelTerrainComponent_h__Script_VoxelTerrain_Statics
struct UHT_STATICS
{
	static constexpr FStructRegisterCompiledInInfo ScriptStructInfo[] = {
		{ Z_Construct_UScriptStruct_FVoxelNoiseLayerInfo, Z_Construct_UScriptStruct_FVoxelNoiseLayerInfo_Statics::NewStructOps, TEXT("VoxelNoiseLayerInfo"),&Z_Registration_Info_UScriptStruct_FVoxelNoiseLayerInfo, CONSTRUCT_RELOAD_VERSION_INFO(FStructReloadVersionInfo, sizeof(FVoxelNoiseLayerInfo), 102991942U) },
		{ Z_Construct_UScriptStruct_FVoxelNoiseLayer3DInfo, Z_Construct_UScriptStruct_FVoxelNoiseLayer3DInfo_Statics::NewStructOps, TEXT("VoxelNoiseLayer3DInfo"),&Z_Registration_Info_UScriptStruct_FVoxelNoiseLayer3DInfo, CONSTRUCT_RELOAD_VERSION_INFO(FStructReloadVersionInfo, sizeof(FVoxelNoiseLayer3DInfo), 2552703445U) },
		{ Z_Construct_UScriptStruct_FVoxelVegetationConfig, Z_Construct_UScriptStruct_FVoxelVegetationConfig_Statics::NewStructOps, TEXT("VoxelVegetationConfig"),&Z_Registration_Info_UScriptStruct_FVoxelVegetationConfig, CONSTRUCT_RELOAD_VERSION_INFO(FStructReloadVersionInfo, sizeof(FVoxelVegetationConfig), 1501088964U) },
	};
	static constexpr FClassRegisterCompiledInInfo ClassInfo[] = {
		{ Z_Construct_UClass_UVoxelTerrainComponent, TEXT("UVoxelTerrainComponent"), &Z_Registration_Info_UClass_UVoxelTerrainComponent, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(UVoxelTerrainComponent), 1050658391U) },
	};
}; // UHT_STATICS 
static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_Administrator_Documents_Unreal_Projects_World_Plugins_VoxelTerrain_Source_VoxelTerrain_Public_VoxelTerrainComponent_h__Script_VoxelTerrain_87a1936094169cf161f2672708db9b9408ae8068{
	TEXT("/Script/VoxelTerrain"),
	UHT_STATICS::ClassInfo, UE_ARRAY_COUNT(UHT_STATICS::ClassInfo),
	UHT_STATICS::ScriptStructInfo, UE_ARRAY_COUNT(UHT_STATICS::ScriptStructInfo),
	nullptr, 0,
	nullptr, 0,
};
#undef UHT_STATICS
// ********** End Registration *********************************************************************
#undef UHT_STRUCT_BASE

PRAGMA_ENABLE_DEPRECATION_WARNINGS
