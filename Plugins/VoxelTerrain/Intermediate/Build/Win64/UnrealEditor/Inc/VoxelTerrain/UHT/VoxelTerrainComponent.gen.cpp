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
ENGINE_API UClass* Z_Construct_UClass_UPrimitiveComponent(ETypeConstructPhase);
ENGINE_API UClass* Z_Construct_UClass_UMaterialInterface(ETypeConstructPhase);
// ********** End Cross Module References **********************************************************

// ********** Begin Same Module References *********************************************************
UPackage* Z_Construct_UPackage__Script_VoxelTerrain(ETypeConstructPhase);
VOXELTERRAIN_API UClass* Z_Construct_UClass_UVoxelTerrainComponent(ETypeConstructPhase);
VOXELTERRAIN_API UClass* Z_Construct_UClass_UVoxelTerrainComponent(ETypeConstructPhase);
// ********** End Same Module References ***********************************************************
#define UHT_STRUCT_BASE(INIT) UE::CodeGen::ConstInit::TCompiledInObjectPtr<const FStructBaseChain>(UE::Private::AsStructBaseChain(INIT))

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
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_LOD0RadiusXY_MetaData[] = {
		{ "Category", "Voxel|LOD" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "// \xe2\xad\x90 LOD0 \xe8\x8c\x83\xe5\x9b\xb4\xef\xbc\x88\xe5\x8d\x95\xe4\xbd\x8d\xef\xbc\x9a\xe5\x8e\x98\xe7\xb1\xb3 CM\xef\xbc\x89\n" },
#endif
		{ "DisplayName", "LOD0 Radius XY (cm)" },
		{ "ModuleRelativePath", "Public/VoxelTerrainComponent.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "\xe2\xad\x90 LOD0 \xe8\x8c\x83\xe5\x9b\xb4\xef\xbc\x88\xe5\x8d\x95\xe4\xbd\x8d\xef\xbc\x9a\xe5\x8e\x98\xe7\xb1\xb3 CM\xef\xbc\x89" },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_LOD0RadiusZ_MetaData[] = {
		{ "Category", "Voxel|LOD" },
		{ "DisplayName", "LOD0 Radius Z (cm)" },
		{ "ModuleRelativePath", "Public/VoxelTerrainComponent.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_LOD1RadiusXY_MetaData[] = {
		{ "Category", "Voxel|LOD" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "// \xe2\xad\x90 LOD1 \xe8\x8c\x83\xe5\x9b\xb4\xef\xbc\x88\xe5\x8d\x95\xe4\xbd\x8d\xef\xbc\x9a\xe5\x8e\x98\xe7\xb1\xb3 CM\xef\xbc\x89\n" },
#endif
		{ "DisplayName", "LOD1 Radius XY (cm)" },
		{ "ModuleRelativePath", "Public/VoxelTerrainComponent.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "\xe2\xad\x90 LOD1 \xe8\x8c\x83\xe5\x9b\xb4\xef\xbc\x88\xe5\x8d\x95\xe4\xbd\x8d\xef\xbc\x9a\xe5\x8e\x98\xe7\xb1\xb3 CM\xef\xbc\x89" },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_LOD1RadiusZ_MetaData[] = {
		{ "Category", "Voxel|LOD" },
		{ "DisplayName", "LOD1 Radius Z (cm)" },
		{ "ModuleRelativePath", "Public/VoxelTerrainComponent.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_CollisionRadius_MetaData[] = {
		{ "Category", "Voxel" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "// \xe7\xa2\xb0\xe6\x92\x9e\xe8\x8c\x83\xe5\x9b\xb4\xef\xbc\x88\xe5\x8d\x95\xe4\xbd\x8d\xef\xbc\x9a\xe5\x8e\x98\xe7\xb1\xb3 CM\xef\xbc\x89\n" },
#endif
		{ "DisplayName", "Collision Radius (cm)" },
		{ "ModuleRelativePath", "Public/VoxelTerrainComponent.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "\xe7\xa2\xb0\xe6\x92\x9e\xe8\x8c\x83\xe5\x9b\xb4\xef\xbc\x88\xe5\x8d\x95\xe4\xbd\x8d\xef\xbc\x9a\xe5\x8e\x98\xe7\xb1\xb3 CM\xef\xbc\x89" },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_UnloadDelay_MetaData[] = {
		{ "Category", "Voxel" },
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
	static const UECodeGen_Private::FFloatPropertyParams NewProp_LOD0RadiusXY;
	static const UECodeGen_Private::FFloatPropertyParams NewProp_LOD0RadiusZ;
	static const UECodeGen_Private::FFloatPropertyParams NewProp_LOD1RadiusXY;
	static const UECodeGen_Private::FFloatPropertyParams NewProp_LOD1RadiusZ;
	static const UECodeGen_Private::FFloatPropertyParams NewProp_CollisionRadius;
	static const UECodeGen_Private::FFloatPropertyParams NewProp_UnloadDelay;
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
const UECodeGen_Private::FFloatPropertyParams UHT_STATICS::NewProp_LOD0RadiusXY = { "LOD0RadiusXY", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Float, nullptr, nullptr, 1, STRUCT_OFFSET(UVoxelTerrainComponent, LOD0RadiusXY), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_LOD0RadiusXY_MetaData), NewProp_LOD0RadiusXY_MetaData) };
const UECodeGen_Private::FFloatPropertyParams UHT_STATICS::NewProp_LOD0RadiusZ = { "LOD0RadiusZ", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Float, nullptr, nullptr, 1, STRUCT_OFFSET(UVoxelTerrainComponent, LOD0RadiusZ), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_LOD0RadiusZ_MetaData), NewProp_LOD0RadiusZ_MetaData) };
const UECodeGen_Private::FFloatPropertyParams UHT_STATICS::NewProp_LOD1RadiusXY = { "LOD1RadiusXY", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Float, nullptr, nullptr, 1, STRUCT_OFFSET(UVoxelTerrainComponent, LOD1RadiusXY), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_LOD1RadiusXY_MetaData), NewProp_LOD1RadiusXY_MetaData) };
const UECodeGen_Private::FFloatPropertyParams UHT_STATICS::NewProp_LOD1RadiusZ = { "LOD1RadiusZ", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Float, nullptr, nullptr, 1, STRUCT_OFFSET(UVoxelTerrainComponent, LOD1RadiusZ), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_LOD1RadiusZ_MetaData), NewProp_LOD1RadiusZ_MetaData) };
const UECodeGen_Private::FFloatPropertyParams UHT_STATICS::NewProp_CollisionRadius = { "CollisionRadius", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Float, nullptr, nullptr, 1, STRUCT_OFFSET(UVoxelTerrainComponent, CollisionRadius), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_CollisionRadius_MetaData), NewProp_CollisionRadius_MetaData) };
const UECodeGen_Private::FFloatPropertyParams UHT_STATICS::NewProp_UnloadDelay = { "UnloadDelay", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Float, nullptr, nullptr, 1, STRUCT_OFFSET(UVoxelTerrainComponent, UnloadDelay), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_UnloadDelay_MetaData), NewProp_UnloadDelay_MetaData) };
const UECodeGen_Private::FPropertyParamsBase* const UHT_STATICS::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_TerrainMaterial,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_bGenerateCollision,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_LOD0RadiusXY,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_LOD0RadiusZ,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_LOD1RadiusXY,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_LOD1RadiusZ,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_CollisionRadius,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_UnloadDelay,
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
	static constexpr FClassRegisterCompiledInInfo ClassInfo[] = {
		{ Z_Construct_UClass_UVoxelTerrainComponent, TEXT("UVoxelTerrainComponent"), &Z_Registration_Info_UClass_UVoxelTerrainComponent, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(UVoxelTerrainComponent), 831721686U) },
	};
}; // UHT_STATICS 
static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_Administrator_Documents_Unreal_Projects_World_Plugins_VoxelTerrain_Source_VoxelTerrain_Public_VoxelTerrainComponent_h__Script_VoxelTerrain_ad090ebfe06e6816f9d68fb14ea17ee0c9365290{
	TEXT("/Script/VoxelTerrain"),
	UHT_STATICS::ClassInfo, UE_ARRAY_COUNT(UHT_STATICS::ClassInfo),
	nullptr, 0,
	nullptr, 0,
	nullptr, 0,
};
#undef UHT_STATICS
// ********** End Registration *********************************************************************
#undef UHT_STRUCT_BASE

PRAGMA_ENABLE_DEPRECATION_WARNINGS
