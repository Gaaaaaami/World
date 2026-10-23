// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "VoxelInteractionActor.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
static_assert(!UE_WITH_CONSTINIT_UOBJECT, "This generated code can only be compiled with !UE_WITH_CONSTINIT_UOBJECT");
void EmptyLinkFunctionForGeneratedCodeVoxelInteractionActor() {}

// ********** Begin Cross Module References ********************************************************
ENGINE_API UClass* Z_Construct_UClass_AActor(ETypeConstructPhase);
// ********** End Cross Module References **********************************************************

// ********** Begin Same Module References *********************************************************
UPackage* Z_Construct_UPackage__Script_VoxelTerrain(ETypeConstructPhase);
VOXELTERRAIN_API UClass* Z_Construct_UClass_AVoxelInteractionActor(ETypeConstructPhase);
VOXELTERRAIN_API UClass* Z_Construct_UClass_AVoxelInteractionActor(ETypeConstructPhase);
// ********** End Same Module References ***********************************************************
#define UHT_STRUCT_BASE(INIT) UE::CodeGen::ConstInit::TCompiledInObjectPtr<const FStructBaseChain>(UE::Private::AsStructBaseChain(INIT))

// ********** Begin Class AVoxelInteractionActor ***************************************************
#ifdef UHT_STATICS
#error UHT_STATICS already defined
#endif
#define UHT_STATICS Z_Construct_UClass_AVoxelInteractionActor_Statics
struct UHT_STATICS
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Type_MetaData[] = {
		{ "IncludePath", "VoxelInteractionActor.h" },
		{ "ModuleRelativePath", "Public/VoxelInteractionActor.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_MaxTraceDistance_MetaData[] = {
		{ "Category", "Voxel Interaction" },
		{ "ModuleRelativePath", "Public/VoxelInteractionActor.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_EditRadius_MetaData[] = {
		{ "Category", "Voxel Interaction" },
		{ "ModuleRelativePath", "Public/VoxelInteractionActor.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_EditCooldown_MetaData[] = {
		{ "Category", "Voxel Interaction" },
		{ "ModuleRelativePath", "Public/VoxelInteractionActor.h" },
	};
#endif // WITH_METADATA

// ********** Begin Class AVoxelInteractionActor constinit property declarations *******************
	static const UECodeGen_Private::FFloatPropertyParams NewProp_MaxTraceDistance;
	static const UECodeGen_Private::FFloatPropertyParams NewProp_EditRadius;
	static const UECodeGen_Private::FFloatPropertyParams NewProp_EditCooldown;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
// ********** End Class AVoxelInteractionActor constinit property declarations *********************
	static FTypeConstructFunc* DependentSingletons[];
	static constexpr FCppClassTypeInfoStatic StaticCppClassTypeInfo = {
		TCppClassTypeTraits<AVoxelInteractionActor>::IsAbstract,
	};
	static const UECodeGen_Private::FClassParams ClassParams;
}; // struct UHT_STATICS

// ********** Begin Class AVoxelInteractionActor Property Definitions ******************************
const UECodeGen_Private::FFloatPropertyParams UHT_STATICS::NewProp_MaxTraceDistance = { "MaxTraceDistance", nullptr, (EPropertyFlags)0x0020080000000001, UECodeGen_Private::EPropertyGenFlags::Float, nullptr, nullptr, 1, STRUCT_OFFSET(AVoxelInteractionActor, MaxTraceDistance), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_MaxTraceDistance_MetaData), NewProp_MaxTraceDistance_MetaData) };
const UECodeGen_Private::FFloatPropertyParams UHT_STATICS::NewProp_EditRadius = { "EditRadius", nullptr, (EPropertyFlags)0x0020080000000001, UECodeGen_Private::EPropertyGenFlags::Float, nullptr, nullptr, 1, STRUCT_OFFSET(AVoxelInteractionActor, EditRadius), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_EditRadius_MetaData), NewProp_EditRadius_MetaData) };
const UECodeGen_Private::FFloatPropertyParams UHT_STATICS::NewProp_EditCooldown = { "EditCooldown", nullptr, (EPropertyFlags)0x0020080000000001, UECodeGen_Private::EPropertyGenFlags::Float, nullptr, nullptr, 1, STRUCT_OFFSET(AVoxelInteractionActor, EditCooldown), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_EditCooldown_MetaData), NewProp_EditCooldown_MetaData) };
const UECodeGen_Private::FPropertyParamsBase* const UHT_STATICS::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_MaxTraceDistance,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_EditRadius,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_EditCooldown,
};
static_assert(UE_ARRAY_COUNT(UHT_STATICS::PropPointers) < 2048);
// ********** End Class AVoxelInteractionActor Property Definitions ********************************
FTypeConstructFunc* UHT_STATICS::DependentSingletons[] = {
	(FTypeConstructFunc*)Z_Construct_UClass_AActor,
	(FTypeConstructFunc*)Z_Construct_UPackage__Script_VoxelTerrain,
};
static_assert(UE_ARRAY_COUNT(UHT_STATICS::DependentSingletons) < 16);
const UECodeGen_Private::FClassParams UHT_STATICS::ClassParams = {
	&Z_Construct_UClass_AVoxelInteractionActor,
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
	0x009000A4u,
	METADATA_PARAMS(UE_ARRAY_COUNT(UHT_STATICS::Type_MetaData), UHT_STATICS::Type_MetaData)
};
FClassRegistrationInfo Z_Registration_Info_UClass_AVoxelInteractionActor;
UClass* Z_Construct_UClass_AVoxelInteractionActor(ETypeConstructPhase Phase)
{
	if (Phase == ETypeConstructPhase::Inner)
	{
		using TClass = AVoxelInteractionActor;
		if (!Z_Registration_Info_UClass_AVoxelInteractionActor.InnerSingleton)
		{
			GetPrivateStaticClassBody(
				TClass::StaticPackage(),
				TEXT("VoxelInteractionActor"),
				Z_Registration_Info_UClass_AVoxelInteractionActor.InnerSingleton,
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
		return Z_Registration_Info_UClass_AVoxelInteractionActor.InnerSingleton;
	}
	if (!Z_Registration_Info_UClass_AVoxelInteractionActor.OuterSingleton)
	{
		UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_AVoxelInteractionActor.OuterSingleton, UHT_STATICS::ClassParams);
	}
	return Z_Registration_Info_UClass_AVoxelInteractionActor.OuterSingleton;
}
#undef UHT_STATICS
DEFINE_VTABLE_PTR_HELPER_CTOR_NS(, AVoxelInteractionActor);
AVoxelInteractionActor::~AVoxelInteractionActor() {}
// ********** End Class AVoxelInteractionActor *****************************************************

// ********** Begin Registration *******************************************************************
#ifdef UHT_STATICS
#error UHT_STATICS already defined
#endif
#define UHT_STATICS Z_CompiledInDeferFile_FID_Users_Administrator_Documents_Unreal_Projects_World_Plugins_VoxelTerrain_Source_VoxelTerrain_Public_VoxelInteractionActor_h__Script_VoxelTerrain_Statics
struct UHT_STATICS
{
	static constexpr FClassRegisterCompiledInInfo ClassInfo[] = {
		{ Z_Construct_UClass_AVoxelInteractionActor, TEXT("AVoxelInteractionActor"), &Z_Registration_Info_UClass_AVoxelInteractionActor, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(AVoxelInteractionActor), 1492035518U) },
	};
}; // UHT_STATICS 
static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_Administrator_Documents_Unreal_Projects_World_Plugins_VoxelTerrain_Source_VoxelTerrain_Public_VoxelInteractionActor_h__Script_VoxelTerrain_27e9e47cd716dd1e17f0db044efa47239c668d98{
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
