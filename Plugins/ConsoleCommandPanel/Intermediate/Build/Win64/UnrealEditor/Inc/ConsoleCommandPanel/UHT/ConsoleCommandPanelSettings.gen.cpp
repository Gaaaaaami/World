// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "ConsoleCommandPanelSettings.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
static_assert(!UE_WITH_CONSTINIT_UOBJECT, "This generated code can only be compiled with !UE_WITH_CONSTINIT_UOBJECT");
void EmptyLinkFunctionForGeneratedCodeConsoleCommandPanelSettings() {}

// ********** Begin Cross Module References ********************************************************
DEVELOPERSETTINGS_API UClass* Z_Construct_UClass_UDeveloperSettings(ETypeConstructPhase);
// ********** End Cross Module References **********************************************************

// ********** Begin Same Module References *********************************************************
UPackage* Z_Construct_UPackage__Script_ConsoleCommandPanel(ETypeConstructPhase);
CONSOLECOMMANDPANEL_API UScriptStruct* Z_Construct_UScriptStruct_FConsoleCommand(ETypeConstructPhase);
CONSOLECOMMANDPANEL_API UClass* Z_Construct_UClass_UConsoleCommandPanelSettings(ETypeConstructPhase);
CONSOLECOMMANDPANEL_API UClass* Z_Construct_UClass_UConsoleCommandPanelSettings(ETypeConstructPhase);
// ********** End Same Module References ***********************************************************
#define UHT_STRUCT_BASE(INIT) UE::CodeGen::ConstInit::TCompiledInObjectPtr<const FStructBaseChain>(UE::Private::AsStructBaseChain(INIT))

// ********** Begin ScriptStruct FConsoleCommand ***************************************************
#ifdef UHT_STATICS
#error UHT_STATICS already defined
#endif
#define UHT_STATICS Z_Construct_UScriptStruct_FConsoleCommand_Statics
struct UHT_STATICS
{
	static inline consteval int32 GetStructSize() { return DataSizeOf<FConsoleCommand>(); }
	static inline consteval int16 GetStructAlignment() { return alignof(FConsoleCommand); }
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Type_MetaData[] = {
		{ "ModuleRelativePath", "Public/ConsoleCommandPanelSettings.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_DisplayName_MetaData[] = {
		{ "Category", "ConsoleCommand" },
		{ "ModuleRelativePath", "Public/ConsoleCommandPanelSettings.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_Command_MetaData[] = {
		{ "Category", "ConsoleCommand" },
		{ "ModuleRelativePath", "Public/ConsoleCommandPanelSettings.h" },
	};
#endif // WITH_METADATA

// ********** Begin ScriptStruct FConsoleCommand constinit property declarations *******************
	static const UECodeGen_Private::FStrPropertyParams NewProp_DisplayName;
	static const UECodeGen_Private::FStrPropertyParams NewProp_Command;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
// ********** End ScriptStruct FConsoleCommand constinit property declarations *********************
	static void* NewStructOps()
	{
		return (UScriptStruct::ICppStructOps*)new UScriptStruct::TCppStructOps<FConsoleCommand>();
	}
	static const UECodeGen_Private::FStructParams StructParams;
}; // struct UHT_STATICS

// ********** Begin ScriptStruct FConsoleCommand Property Definitions ******************************
const UECodeGen_Private::FStrPropertyParams UHT_STATICS::NewProp_DisplayName = { "DisplayName", nullptr, (EPropertyFlags)0x0010000000004001, UECodeGen_Private::EPropertyGenFlags::Str, nullptr, nullptr, 1, STRUCT_OFFSET(FConsoleCommand, DisplayName), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_DisplayName_MetaData), NewProp_DisplayName_MetaData) };
const UECodeGen_Private::FStrPropertyParams UHT_STATICS::NewProp_Command = { "Command", nullptr, (EPropertyFlags)0x0010000000004001, UECodeGen_Private::EPropertyGenFlags::Str, nullptr, nullptr, 1, STRUCT_OFFSET(FConsoleCommand, Command), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_Command_MetaData), NewProp_Command_MetaData) };
const UECodeGen_Private::FPropertyParamsBase* const UHT_STATICS::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_DisplayName,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_Command,
};
static_assert(UE_ARRAY_COUNT(UHT_STATICS::PropPointers) < 2048);
// ********** End ScriptStruct FConsoleCommand Property Definitions ********************************
const UECodeGen_Private::FStructParams UHT_STATICS::StructParams = {
	(FTypeConstructFunc*)Z_Construct_UPackage__Script_ConsoleCommandPanel,
	nullptr,
	&NewStructOps,
	"ConsoleCommand",
	UHT_STATICS::PropPointers,
	UE_ARRAY_COUNT(UHT_STATICS::PropPointers),
	DataSizeOf<FConsoleCommand>(),
	alignof(FConsoleCommand),
	RF_Public|RF_Transient|RF_MarkAsNative,
	EStructFlags(0x00000001),
	METADATA_PARAMS(UE_ARRAY_COUNT(UHT_STATICS::Type_MetaData), UHT_STATICS::Type_MetaData)
};
static FStructRegistrationInfo Z_Registration_Info_UScriptStruct_FConsoleCommand;
UScriptStruct* Z_Construct_UScriptStruct_FConsoleCommand(ETypeConstructPhase Phase)
{
	if (Phase == ETypeConstructPhase::Outer)
	{
		if (!Z_Registration_Info_UScriptStruct_FConsoleCommand.OuterSingleton)
		{
			Z_Registration_Info_UScriptStruct_FConsoleCommand.OuterSingleton = GetStaticStruct(Z_Construct_UScriptStruct_FConsoleCommand, (UObject*)Z_Construct_UPackage__Script_ConsoleCommandPanel(ETypeConstructPhase::Outer), TEXT("ConsoleCommand"));
		}
		return Z_Registration_Info_UScriptStruct_FConsoleCommand.OuterSingleton;
	}
	if (!Z_Registration_Info_UScriptStruct_FConsoleCommand.InnerSingleton)
	{
		UECodeGen_Private::ConstructUScriptStruct(Z_Registration_Info_UScriptStruct_FConsoleCommand.InnerSingleton, UHT_STATICS::StructParams);
	}
	return CastChecked<UScriptStruct>(Z_Registration_Info_UScriptStruct_FConsoleCommand.InnerSingleton);
}
#undef UHT_STATICS
// ********** End ScriptStruct FConsoleCommand *****************************************************

// ********** Begin Class UConsoleCommandPanelSettings *********************************************
#ifdef UHT_STATICS
#error UHT_STATICS already defined
#endif
#define UHT_STATICS Z_Construct_UClass_UConsoleCommandPanelSettings_Statics
struct UHT_STATICS
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Type_MetaData[] = {
		{ "IncludePath", "ConsoleCommandPanelSettings.h" },
		{ "ModuleRelativePath", "Public/ConsoleCommandPanelSettings.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_Commands_MetaData[] = {
		{ "Category", "ConsoleCommandPanel" },
		{ "ModuleRelativePath", "Public/ConsoleCommandPanelSettings.h" },
	};
#endif // WITH_METADATA

// ********** Begin Class UConsoleCommandPanelSettings constinit property declarations *************
	static const UECodeGen_Private::FStructPropertyParams NewProp_Commands_Inner;
	static const UECodeGen_Private::FArrayPropertyParams NewProp_Commands;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
// ********** End Class UConsoleCommandPanelSettings constinit property declarations ***************
	static FTypeConstructFunc* DependentSingletons[];
	static constexpr FCppClassTypeInfoStatic StaticCppClassTypeInfo = {
		TCppClassTypeTraits<UConsoleCommandPanelSettings>::IsAbstract,
	};
	static const UECodeGen_Private::FClassParams ClassParams;
}; // struct UHT_STATICS

// ********** Begin Class UConsoleCommandPanelSettings Property Definitions ************************
const UECodeGen_Private::FStructPropertyParams UHT_STATICS::NewProp_Commands_Inner = { "Commands", nullptr, (EPropertyFlags)0x0000000000004000, UECodeGen_Private::EPropertyGenFlags::Struct, nullptr, nullptr, 1, 0, Z_Construct_UScriptStruct_FConsoleCommand, METADATA_PARAMS(0, nullptr) }; // 374081b00cc7cad35d5f0a54cc1ed2e015affc82
const UECodeGen_Private::FArrayPropertyParams UHT_STATICS::NewProp_Commands = { "Commands", nullptr, (EPropertyFlags)0x0010000000004001, UECodeGen_Private::EPropertyGenFlags::Array, nullptr, nullptr, 1, STRUCT_OFFSET(UConsoleCommandPanelSettings, Commands), EArrayPropertyFlags::None, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_Commands_MetaData), NewProp_Commands_MetaData) }; // 374081b00cc7cad35d5f0a54cc1ed2e015affc82
const UECodeGen_Private::FPropertyParamsBase* const UHT_STATICS::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_Commands_Inner,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_Commands,
};
static_assert(UE_ARRAY_COUNT(UHT_STATICS::PropPointers) < 2048);
// ********** End Class UConsoleCommandPanelSettings Property Definitions **************************
FTypeConstructFunc* UHT_STATICS::DependentSingletons[] = {
	(FTypeConstructFunc*)Z_Construct_UClass_UDeveloperSettings,
	(FTypeConstructFunc*)Z_Construct_UPackage__Script_ConsoleCommandPanel,
};
static_assert(UE_ARRAY_COUNT(UHT_STATICS::DependentSingletons) < 16);
const UECodeGen_Private::FClassParams UHT_STATICS::ClassParams = {
	&Z_Construct_UClass_UConsoleCommandPanelSettings,
	"EditorPerProjectUserSettings",
	&StaticCppClassTypeInfo,
	DependentSingletons,
	nullptr,
	UHT_STATICS::PropPointers,
	nullptr,
	UE_ARRAY_COUNT(DependentSingletons),
	0,
	UE_ARRAY_COUNT(UHT_STATICS::PropPointers),
	0,
	0x001000A4u,
	METADATA_PARAMS(UE_ARRAY_COUNT(UHT_STATICS::Type_MetaData), UHT_STATICS::Type_MetaData)
};
FClassRegistrationInfo Z_Registration_Info_UClass_UConsoleCommandPanelSettings;
UClass* Z_Construct_UClass_UConsoleCommandPanelSettings(ETypeConstructPhase Phase)
{
	if (Phase == ETypeConstructPhase::Inner)
	{
		using TClass = UConsoleCommandPanelSettings;
		if (!Z_Registration_Info_UClass_UConsoleCommandPanelSettings.InnerSingleton)
		{
			GetPrivateStaticClassBody(
				TClass::StaticPackage(),
				TEXT("ConsoleCommandPanelSettings"),
				Z_Registration_Info_UClass_UConsoleCommandPanelSettings.InnerSingleton,
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
		return Z_Registration_Info_UClass_UConsoleCommandPanelSettings.InnerSingleton;
	}
	if (!Z_Registration_Info_UClass_UConsoleCommandPanelSettings.OuterSingleton)
	{
		UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_UConsoleCommandPanelSettings.OuterSingleton, UHT_STATICS::ClassParams);
	}
	return Z_Registration_Info_UClass_UConsoleCommandPanelSettings.OuterSingleton;
}
#undef UHT_STATICS
UConsoleCommandPanelSettings::UConsoleCommandPanelSettings(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
DEFINE_VTABLE_PTR_HELPER_CTOR_NS(, UConsoleCommandPanelSettings);
UConsoleCommandPanelSettings::~UConsoleCommandPanelSettings() {}
// ********** End Class UConsoleCommandPanelSettings ***********************************************

// ********** Begin Registration *******************************************************************
#ifdef UHT_STATICS
#error UHT_STATICS already defined
#endif
#define UHT_STATICS Z_CompiledInDeferFile_FID_Users_Administrator_Documents_Unreal_Projects_World_Plugins_ConsoleCommandPanel_Source_ConsoleCommandPanel_Public_ConsoleCommandPanelSettings_h__Script_ConsoleCommandPanel_Statics
struct UHT_STATICS
{
	static constexpr FStructRegisterCompiledInInfo ScriptStructInfo[] = {
		{ Z_Construct_UScriptStruct_FConsoleCommand, Z_Construct_UScriptStruct_FConsoleCommand_Statics::NewStructOps, TEXT("ConsoleCommand"),&Z_Registration_Info_UScriptStruct_FConsoleCommand, CONSTRUCT_RELOAD_VERSION_INFO(FStructReloadVersionInfo, sizeof(FConsoleCommand), 926974384U) },
	};
	static constexpr FClassRegisterCompiledInInfo ClassInfo[] = {
		{ Z_Construct_UClass_UConsoleCommandPanelSettings, TEXT("UConsoleCommandPanelSettings"), &Z_Registration_Info_UClass_UConsoleCommandPanelSettings, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(UConsoleCommandPanelSettings), 281764930U) },
	};
}; // UHT_STATICS 
static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_Administrator_Documents_Unreal_Projects_World_Plugins_ConsoleCommandPanel_Source_ConsoleCommandPanel_Public_ConsoleCommandPanelSettings_h__Script_ConsoleCommandPanel_c54a926417c08e495796513fc41d07e094f2a38b{
	TEXT("/Script/ConsoleCommandPanel"),
	UHT_STATICS::ClassInfo, UE_ARRAY_COUNT(UHT_STATICS::ClassInfo),
	UHT_STATICS::ScriptStructInfo, UE_ARRAY_COUNT(UHT_STATICS::ScriptStructInfo),
	nullptr, 0,
	nullptr, 0,
};
#undef UHT_STATICS
// ********** End Registration *********************************************************************
#undef UHT_STRUCT_BASE

PRAGMA_ENABLE_DEPRECATION_WARNINGS
