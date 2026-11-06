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
COREUOBJECT_API UScriptStruct* Z_Construct_UScriptStruct_FVector(ETypeConstructPhase);
COREUOBJECT_API UScriptStruct* Z_Construct_UScriptStruct_FVector2D(ETypeConstructPhase);
ENGINE_API UClass* Z_Construct_UClass_UPrimitiveComponent(ETypeConstructPhase);
ENGINE_API UClass* Z_Construct_UClass_UMaterialInterface(ETypeConstructPhase);
ENGINE_API UClass* Z_Construct_UClass_UTexture2D(ETypeConstructPhase);
ENGINE_API UClass* Z_Construct_UClass_UVolumeTexture(ETypeConstructPhase);
// ********** End Cross Module References **********************************************************

// ********** Begin Same Module References *********************************************************
UPackage* Z_Construct_UPackage__Script_VoxelTerrain(ETypeConstructPhase);
VOXELTERRAIN_API UScriptStruct* Z_Construct_UScriptStruct_FVoxelNoiseLayer3DInfo(ETypeConstructPhase);
VOXELTERRAIN_API UScriptStruct* Z_Construct_UScriptStruct_FVoxelNoiseLayerInfo(ETypeConstructPhase);
VOXELTERRAIN_API UClass* Z_Construct_UClass_UVoxelTerrainComponent(ETypeConstructPhase);
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
#if !UE_BUILD_SHIPPING
		{ "Comment", "// \xe9\xa2\x91\xe7\x8e\x87\xef\xbc\x9a\xe8\xb6\x8a\xe5\xb0\x8f\xe8\xb4\xb4\xe5\x9b\xbe\xe9\x93\xba\xe5\xbe\x97\xe8\xb6\x8a\xe5\xa4\xa7\n" },
#endif
		{ "ModuleRelativePath", "Public/VoxelTerrainComponent.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "\xe9\xa2\x91\xe7\x8e\x87\xef\xbc\x9a\xe8\xb6\x8a\xe5\xb0\x8f\xe8\xb4\xb4\xe5\x9b\xbe\xe9\x93\xba\xe5\xbe\x97\xe8\xb6\x8a\xe5\xa4\xa7" },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_Offset_MetaData[] = {
		{ "Category", "Noise" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "// \xe5\xb9\x85\xe5\xba\xa6\xef\xbc\x9a\xe8\xb4\xa1\xe7\x8c\xae\xe7\x9a\x84\xe9\xab\x98\xe5\xba\xa6\xe6\x9d\x83\xe9\x87\x8d\n" },
#endif
		{ "ModuleRelativePath", "Public/VoxelTerrainComponent.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "\xe5\xb9\x85\xe5\xba\xa6\xef\xbc\x9a\xe8\xb4\xa1\xe7\x8c\xae\xe7\x9a\x84\xe9\xab\x98\xe5\xba\xa6\xe6\x9d\x83\xe9\x87\x8d" },
#endif
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
#if !UE_BUILD_SHIPPING
		{ "Comment", "// \xe2\xad\x90 \xe6\xb3\xa8\xe6\x84\x8f\xef\xbc\x9a\xe8\xbf\x99\xe9\x87\x8c\xe7\x9a\x84\xe7\xb1\xbb\xe5\x9e\x8b\xe6\x98\xaf UTexture3D\n" },
#endif
		{ "ModuleRelativePath", "Public/VoxelTerrainComponent.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "\xe2\xad\x90 \xe6\xb3\xa8\xe6\x84\x8f\xef\xbc\x9a\xe8\xbf\x99\xe9\x87\x8c\xe7\x9a\x84\xe7\xb1\xbb\xe5\x9e\x8b\xe6\x98\xaf UTexture3D" },
#endif
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

// ********** Begin Class UVoxelTerrainComponent Function ClearAllSaveData *************************
#ifdef UHT_STATICS
#error UHT_STATICS already defined
#endif
#define UHT_STATICS Z_Construct_UFunction_UVoxelTerrainComponent_ClearAllSaveData_Statics
struct UHT_STATICS
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Type_MetaData[] = {
		{ "Category", "Voxel" },
		{ "ModuleRelativePath", "Public/VoxelTerrainComponent.h" },
	};
#endif // WITH_METADATA

// ********** Begin Function ClearAllSaveData constinit property declarations **********************
// ********** End Function ClearAllSaveData constinit property declarations ************************
	static const UECodeGen_Private::FFunctionParams FuncParams;
};
const UECodeGen_Private::FFunctionParams UHT_STATICS::FuncParams = { { (FTypeConstructFunc*)Z_Construct_UClass_UVoxelTerrainComponent, nullptr, "ClearAllSaveData", nullptr, 0, 0, RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x04020401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(UHT_STATICS::Type_MetaData), UHT_STATICS::Type_MetaData)},  };
UFunction* Z_Construct_UFunction_UVoxelTerrainComponent_ClearAllSaveData(ETypeConstructPhase Phase)
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, UHT_STATICS::FuncParams);
	}
	return ReturnFunction;
}
#undef UHT_STATICS
DEFINE_FUNCTION(UVoxelTerrainComponent::execClearAllSaveData)
{
	P_FINISH;
	P_NATIVE_BEGIN;
	P_THIS->ClearAllSaveData();
	P_NATIVE_END;
}
// ********** End Class UVoxelTerrainComponent Function ClearAllSaveData ***************************

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
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_bGenerateCollision_MetaData[] = {
		{ "Category", "Voxel" },
		{ "ModuleRelativePath", "Public/VoxelTerrainComponent.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_LOD0RadiusChunksXY_MetaData[] = {
		{ "Category", "Voxel|LOD" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "// \xe2\xad\x90 \xe6\x94\xb9\xe4\xb8\xba\xe6\x8c\x89 Chunk \xe6\x95\xb0\xe9\x87\x8f\xe5\x8a\xa0\xe8\xbd\xbd\xef\xbc\x8c\xe4\xb8\x8d\xe5\x86\x8d\xe5\x8f\x97 VoxelSize \xe5\xbd\xb1\xe5\x93\x8d\n" },
#endif
		{ "DisplayName", "LOD0 Radius XY (Chunks)" },
		{ "ModuleRelativePath", "Public/VoxelTerrainComponent.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "\xe2\xad\x90 \xe6\x94\xb9\xe4\xb8\xba\xe6\x8c\x89 Chunk \xe6\x95\xb0\xe9\x87\x8f\xe5\x8a\xa0\xe8\xbd\xbd\xef\xbc\x8c\xe4\xb8\x8d\xe5\x86\x8d\xe5\x8f\x97 VoxelSize \xe5\xbd\xb1\xe5\x93\x8d" },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_LOD0RadiusChunksZ_MetaData[] = {
		{ "Category", "Voxel|LOD" },
		{ "DisplayName", "LOD0 Radius Z (Chunks)" },
		{ "ModuleRelativePath", "Public/VoxelTerrainComponent.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_CollisionRadiusChunks_MetaData[] = {
		{ "Category", "Voxel" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "// \xe2\xad\x90 \xe7\xa2\xb0\xe6\x92\x9e\xe8\x8c\x83\xe5\x9b\xb4\xef\xbc\x88\xe5\x8d\x95\xe4\xbd\x8d\xef\xbc\x9a""Chunk \xe6\x95\xb0\xe9\x87\x8f\xef\xbc\x89\n" },
#endif
		{ "DisplayName", "Collision Radius (Chunks)" },
		{ "ModuleRelativePath", "Public/VoxelTerrainComponent.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "\xe2\xad\x90 \xe7\xa2\xb0\xe6\x92\x9e\xe8\x8c\x83\xe5\x9b\xb4\xef\xbc\x88\xe5\x8d\x95\xe4\xbd\x8d\xef\xbc\x9a""Chunk \xe6\x95\xb0\xe9\x87\x8f\xef\xbc\x89" },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_UnloadDelay_MetaData[] = {
		{ "Category", "Voxel" },
		{ "ModuleRelativePath", "Public/VoxelTerrainComponent.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_CollisionKeepDuration_MetaData[] = {
		{ "Category", "Voxel" },
		{ "DisplayName", "Collision Keep Duration (s)" },
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
#endif // WITH_METADATA

// ********** Begin Class UVoxelTerrainComponent constinit property declarations *******************
	static const UECodeGen_Private::FObjectPropertyParams NewProp_TerrainMaterial;
	static void NewProp_bGenerateCollision_SetBit(void* Obj)
	{
		((UVoxelTerrainComponent*)Obj)->bGenerateCollision = 1;
	}
	static const UECodeGen_Private::FBoolPropertyParams NewProp_bGenerateCollision;
	static const UECodeGen_Private::FIntPropertyParams NewProp_LOD0RadiusChunksXY;
	static const UECodeGen_Private::FIntPropertyParams NewProp_LOD0RadiusChunksZ;
	static const UECodeGen_Private::FIntPropertyParams NewProp_CollisionRadiusChunks;
	static const UECodeGen_Private::FFloatPropertyParams NewProp_UnloadDelay;
	static const UECodeGen_Private::FFloatPropertyParams NewProp_CollisionKeepDuration;
	static const UECodeGen_Private::FStructPropertyParams NewProp_NoiseLayers_Inner;
	static const UECodeGen_Private::FArrayPropertyParams NewProp_NoiseLayers;
	static const UECodeGen_Private::FStructPropertyParams NewProp_NoiseLayers3D_Inner;
	static const UECodeGen_Private::FArrayPropertyParams NewProp_NoiseLayers3D;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
// ********** End Class UVoxelTerrainComponent constinit property declarations *********************
	static constexpr UE::CodeGen::FClassNativeFunction Funcs[] = {
		{ .NameUTF8 = UTF8TEXT("ApplyEdit"), .Pointer = &UVoxelTerrainComponent::execApplyEdit },
		{ .NameUTF8 = UTF8TEXT("ClearAllSaveData"), .Pointer = &UVoxelTerrainComponent::execClearAllSaveData },
	};
	static FTypeConstructFunc* DependentSingletons[];
	static constexpr FClassFunctionLinkInfo FuncInfo[] = {
		{ &Z_Construct_UFunction_UVoxelTerrainComponent_ApplyEdit, "ApplyEdit" }, // c2436e9c6f20010e7cf1f092b5a7ef761a69d398
		{ &Z_Construct_UFunction_UVoxelTerrainComponent_ClearAllSaveData, "ClearAllSaveData" }, // daa29b6d64eef781a25a9c636dcfd5979fa8f492
	};
	static_assert(UE_ARRAY_COUNT(FuncInfo) < 2048);
	static constexpr FCppClassTypeInfoStatic StaticCppClassTypeInfo = {
		TCppClassTypeTraits<UVoxelTerrainComponent>::IsAbstract,
	};
	static const UECodeGen_Private::FClassParams ClassParams;
}; // struct UHT_STATICS

// ********** Begin Class UVoxelTerrainComponent Property Definitions ******************************
const UECodeGen_Private::FObjectPropertyParams UHT_STATICS::NewProp_TerrainMaterial = { "TerrainMaterial", nullptr, (EPropertyFlags)0x0114000000000001, UECodeGen_Private::EPropertyGenFlags::Object | UECodeGen_Private::EPropertyGenFlags::ObjectPtr, nullptr, nullptr, 1, STRUCT_OFFSET(UVoxelTerrainComponent, TerrainMaterial), Z_Construct_UClass_UMaterialInterface, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_TerrainMaterial_MetaData), NewProp_TerrainMaterial_MetaData) };
const UECodeGen_Private::FBoolPropertyParams UHT_STATICS::NewProp_bGenerateCollision = { "bGenerateCollision", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Bool | UECodeGen_Private::EPropertyGenFlags::NativeBool, nullptr, nullptr, 1, sizeof(bool), sizeof(UVoxelTerrainComponent), &UHT_STATICS::NewProp_bGenerateCollision_SetBit, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_bGenerateCollision_MetaData), NewProp_bGenerateCollision_MetaData) };
const UECodeGen_Private::FIntPropertyParams UHT_STATICS::NewProp_LOD0RadiusChunksXY = { "LOD0RadiusChunksXY", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Int, nullptr, nullptr, 1, STRUCT_OFFSET(UVoxelTerrainComponent, LOD0RadiusChunksXY), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_LOD0RadiusChunksXY_MetaData), NewProp_LOD0RadiusChunksXY_MetaData) };
const UECodeGen_Private::FIntPropertyParams UHT_STATICS::NewProp_LOD0RadiusChunksZ = { "LOD0RadiusChunksZ", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Int, nullptr, nullptr, 1, STRUCT_OFFSET(UVoxelTerrainComponent, LOD0RadiusChunksZ), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_LOD0RadiusChunksZ_MetaData), NewProp_LOD0RadiusChunksZ_MetaData) };
const UECodeGen_Private::FIntPropertyParams UHT_STATICS::NewProp_CollisionRadiusChunks = { "CollisionRadiusChunks", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Int, nullptr, nullptr, 1, STRUCT_OFFSET(UVoxelTerrainComponent, CollisionRadiusChunks), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_CollisionRadiusChunks_MetaData), NewProp_CollisionRadiusChunks_MetaData) };
const UECodeGen_Private::FFloatPropertyParams UHT_STATICS::NewProp_UnloadDelay = { "UnloadDelay", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Float, nullptr, nullptr, 1, STRUCT_OFFSET(UVoxelTerrainComponent, UnloadDelay), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_UnloadDelay_MetaData), NewProp_UnloadDelay_MetaData) };
const UECodeGen_Private::FFloatPropertyParams UHT_STATICS::NewProp_CollisionKeepDuration = { "CollisionKeepDuration", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Float, nullptr, nullptr, 1, STRUCT_OFFSET(UVoxelTerrainComponent, CollisionKeepDuration), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_CollisionKeepDuration_MetaData), NewProp_CollisionKeepDuration_MetaData) };
const UECodeGen_Private::FStructPropertyParams UHT_STATICS::NewProp_NoiseLayers_Inner = { "NoiseLayers", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Struct, nullptr, nullptr, 1, 0, Z_Construct_UScriptStruct_FVoxelNoiseLayerInfo, METADATA_PARAMS(0, nullptr) }; // 7bc80d840451a14c895b50c4934ca0fb18b7a3cd
const UECodeGen_Private::FArrayPropertyParams UHT_STATICS::NewProp_NoiseLayers = { "NoiseLayers", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Array, nullptr, nullptr, 1, STRUCT_OFFSET(UVoxelTerrainComponent, NoiseLayers), EArrayPropertyFlags::None, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_NoiseLayers_MetaData), NewProp_NoiseLayers_MetaData) }; // 7bc80d840451a14c895b50c4934ca0fb18b7a3cd
const UECodeGen_Private::FStructPropertyParams UHT_STATICS::NewProp_NoiseLayers3D_Inner = { "NoiseLayers3D", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Struct, nullptr, nullptr, 1, 0, Z_Construct_UScriptStruct_FVoxelNoiseLayer3DInfo, METADATA_PARAMS(0, nullptr) }; // 9856a57d3f48d28ba1de55c90d5b750d7dca503d
const UECodeGen_Private::FArrayPropertyParams UHT_STATICS::NewProp_NoiseLayers3D = { "NoiseLayers3D", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Array, nullptr, nullptr, 1, STRUCT_OFFSET(UVoxelTerrainComponent, NoiseLayers3D), EArrayPropertyFlags::None, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_NoiseLayers3D_MetaData), NewProp_NoiseLayers3D_MetaData) }; // 9856a57d3f48d28ba1de55c90d5b750d7dca503d
const UECodeGen_Private::FPropertyParamsBase* const UHT_STATICS::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_TerrainMaterial,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_bGenerateCollision,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_LOD0RadiusChunksXY,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_LOD0RadiusChunksZ,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_CollisionRadiusChunks,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_UnloadDelay,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_CollisionKeepDuration,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_NoiseLayers_Inner,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_NoiseLayers,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_NoiseLayers3D_Inner,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_NoiseLayers3D,
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
		{ Z_Construct_UScriptStruct_FVoxelNoiseLayerInfo, Z_Construct_UScriptStruct_FVoxelNoiseLayerInfo_Statics::NewStructOps, TEXT("VoxelNoiseLayerInfo"),&Z_Registration_Info_UScriptStruct_FVoxelNoiseLayerInfo, CONSTRUCT_RELOAD_VERSION_INFO(FStructReloadVersionInfo, sizeof(FVoxelNoiseLayerInfo), 2076708228U) },
		{ Z_Construct_UScriptStruct_FVoxelNoiseLayer3DInfo, Z_Construct_UScriptStruct_FVoxelNoiseLayer3DInfo_Statics::NewStructOps, TEXT("VoxelNoiseLayer3DInfo"),&Z_Registration_Info_UScriptStruct_FVoxelNoiseLayer3DInfo, CONSTRUCT_RELOAD_VERSION_INFO(FStructReloadVersionInfo, sizeof(FVoxelNoiseLayer3DInfo), 2555815293U) },
	};
	static constexpr FClassRegisterCompiledInInfo ClassInfo[] = {
		{ Z_Construct_UClass_UVoxelTerrainComponent, TEXT("UVoxelTerrainComponent"), &Z_Registration_Info_UClass_UVoxelTerrainComponent, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(UVoxelTerrainComponent), 3512352847U) },
	};
}; // UHT_STATICS 
static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_Administrator_Documents_Unreal_Projects_World_Plugins_VoxelTerrain_Source_VoxelTerrain_Public_VoxelTerrainComponent_h__Script_VoxelTerrain_a28b619b17214552f6cd571bebd9a753bec96b54{
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
