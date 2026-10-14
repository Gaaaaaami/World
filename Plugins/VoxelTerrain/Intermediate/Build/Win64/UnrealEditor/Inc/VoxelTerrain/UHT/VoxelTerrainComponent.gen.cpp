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
ENGINE_API UClass* Z_Construct_UClass_UPrimitiveComponent(ETypeConstructPhase);
ENGINE_API UClass* Z_Construct_UClass_UMaterialInterface(ETypeConstructPhase);
PROCEDURALMESHCOMPONENT_API UClass* Z_Construct_UClass_UProceduralMeshComponent(ETypeConstructPhase);
// ********** End Cross Module References **********************************************************

// ********** Begin Same Module References *********************************************************
UPackage* Z_Construct_UPackage__Script_VoxelTerrain(ETypeConstructPhase);
VOXELTERRAIN_API UClass* Z_Construct_UClass_UVoxelTerrainComponent(ETypeConstructPhase);
VOXELTERRAIN_API UClass* Z_Construct_UClass_UVoxelTerrainComponent(ETypeConstructPhase);
// ********** End Same Module References ***********************************************************
#define UHT_STRUCT_BASE(INIT) UE::CodeGen::ConstInit::TCompiledInObjectPtr<const FStructBaseChain>(UE::Private::AsStructBaseChain(INIT))

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
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_ViewRadius_MetaData[] = {
		{ "Category", "Voxel" },
		{ "ModuleRelativePath", "Public/VoxelTerrainComponent.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_UnloadRadius_MetaData[] = {
		{ "Category", "Voxel" },
		{ "ModuleRelativePath", "Public/VoxelTerrainComponent.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_LOD0Radius_MetaData[] = {
		{ "Category", "Voxel" },
		{ "ModuleRelativePath", "Public/VoxelTerrainComponent.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_LOD1Radius_MetaData[] = {
		{ "Category", "Voxel" },
		{ "ModuleRelativePath", "Public/VoxelTerrainComponent.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_UnloadDelay_MetaData[] = {
		{ "Category", "Voxel" },
		{ "ModuleRelativePath", "Public/VoxelTerrainComponent.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_CollisionProxy_MetaData[] = {
		{ "EditInline", "true" },
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
	static const UECodeGen_Private::FIntPropertyParams NewProp_ViewRadius;
	static const UECodeGen_Private::FIntPropertyParams NewProp_UnloadRadius;
	static const UECodeGen_Private::FIntPropertyParams NewProp_LOD0Radius;
	static const UECodeGen_Private::FIntPropertyParams NewProp_LOD1Radius;
	static const UECodeGen_Private::FFloatPropertyParams NewProp_UnloadDelay;
	static const UECodeGen_Private::FObjectPropertyParams NewProp_CollisionProxy;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
// ********** End Class UVoxelTerrainComponent constinit property declarations *********************
	static FTypeConstructFunc* DependentSingletons[];
	static constexpr FCppClassTypeInfoStatic StaticCppClassTypeInfo = {
		TCppClassTypeTraits<UVoxelTerrainComponent>::IsAbstract,
	};
	static const UECodeGen_Private::FClassParams ClassParams;
}; // struct UHT_STATICS

// ********** Begin Class UVoxelTerrainComponent Property Definitions ******************************
const UECodeGen_Private::FObjectPropertyParams UHT_STATICS::NewProp_TerrainMaterial = { "TerrainMaterial", nullptr, (EPropertyFlags)0x0114000000000001, UECodeGen_Private::EPropertyGenFlags::Object | UECodeGen_Private::EPropertyGenFlags::ObjectPtr, nullptr, nullptr, 1, STRUCT_OFFSET(UVoxelTerrainComponent, TerrainMaterial), Z_Construct_UClass_UMaterialInterface, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_TerrainMaterial_MetaData), NewProp_TerrainMaterial_MetaData) };
const UECodeGen_Private::FBoolPropertyParams UHT_STATICS::NewProp_bGenerateCollision = { "bGenerateCollision", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Bool | UECodeGen_Private::EPropertyGenFlags::NativeBool, nullptr, nullptr, 1, sizeof(bool), sizeof(UVoxelTerrainComponent), &UHT_STATICS::NewProp_bGenerateCollision_SetBit, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_bGenerateCollision_MetaData), NewProp_bGenerateCollision_MetaData) };
const UECodeGen_Private::FIntPropertyParams UHT_STATICS::NewProp_ViewRadius = { "ViewRadius", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Int, nullptr, nullptr, 1, STRUCT_OFFSET(UVoxelTerrainComponent, ViewRadius), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_ViewRadius_MetaData), NewProp_ViewRadius_MetaData) };
const UECodeGen_Private::FIntPropertyParams UHT_STATICS::NewProp_UnloadRadius = { "UnloadRadius", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Int, nullptr, nullptr, 1, STRUCT_OFFSET(UVoxelTerrainComponent, UnloadRadius), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_UnloadRadius_MetaData), NewProp_UnloadRadius_MetaData) };
const UECodeGen_Private::FIntPropertyParams UHT_STATICS::NewProp_LOD0Radius = { "LOD0Radius", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Int, nullptr, nullptr, 1, STRUCT_OFFSET(UVoxelTerrainComponent, LOD0Radius), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_LOD0Radius_MetaData), NewProp_LOD0Radius_MetaData) };
const UECodeGen_Private::FIntPropertyParams UHT_STATICS::NewProp_LOD1Radius = { "LOD1Radius", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Int, nullptr, nullptr, 1, STRUCT_OFFSET(UVoxelTerrainComponent, LOD1Radius), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_LOD1Radius_MetaData), NewProp_LOD1Radius_MetaData) };
const UECodeGen_Private::FFloatPropertyParams UHT_STATICS::NewProp_UnloadDelay = { "UnloadDelay", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Float, nullptr, nullptr, 1, STRUCT_OFFSET(UVoxelTerrainComponent, UnloadDelay), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_UnloadDelay_MetaData), NewProp_UnloadDelay_MetaData) };
const UECodeGen_Private::FObjectPropertyParams UHT_STATICS::NewProp_CollisionProxy = { "CollisionProxy", nullptr, (EPropertyFlags)0x0144000000080008, UECodeGen_Private::EPropertyGenFlags::Object | UECodeGen_Private::EPropertyGenFlags::ObjectPtr, nullptr, nullptr, 1, STRUCT_OFFSET(UVoxelTerrainComponent, CollisionProxy), Z_Construct_UClass_UProceduralMeshComponent, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_CollisionProxy_MetaData), NewProp_CollisionProxy_MetaData) };
const UECodeGen_Private::FPropertyParamsBase* const UHT_STATICS::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_TerrainMaterial,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_bGenerateCollision,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_ViewRadius,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_UnloadRadius,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_LOD0Radius,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_LOD1Radius,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_UnloadDelay,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_CollisionProxy,
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
	nullptr,
	UHT_STATICS::PropPointers,
	nullptr,
	UE_ARRAY_COUNT(DependentSingletons),
	0,
	UE_ARRAY_COUNT(UHT_STATICS::PropPointers),
	0,
	0x00B000A4u,
	METADATA_PARAMS(UE_ARRAY_COUNT(UHT_STATICS::Type_MetaData), UHT_STATICS::Type_MetaData)
};
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
				nullptr,
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
		{ Z_Construct_UClass_UVoxelTerrainComponent, TEXT("UVoxelTerrainComponent"), &Z_Registration_Info_UClass_UVoxelTerrainComponent, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(UVoxelTerrainComponent), 365247399U) },
	};
}; // UHT_STATICS 
static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_Administrator_Documents_Unreal_Projects_World_Plugins_VoxelTerrain_Source_VoxelTerrain_Public_VoxelTerrainComponent_h__Script_VoxelTerrain_68c0368a762dbb46b4bed903c1f8f548e40c326d{
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
