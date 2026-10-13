// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

// IWYU pragma: private, include "ConsoleCommandPanelSettings.h"

#ifdef CONSOLECOMMANDPANEL_ConsoleCommandPanelSettings_generated_h
#error "ConsoleCommandPanelSettings.generated.h already included, missing '#pragma once' in ConsoleCommandPanelSettings.h"
#endif
#define CONSOLECOMMANDPANEL_ConsoleCommandPanelSettings_generated_h

#include "UObject/ObjectMacros.h"
#include "UObject/ReflectedTypeAccessors.h"
#include "UObject/ScriptMacros.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS

// ********** Begin ScriptStruct FConsoleCommand ***************************************************
struct Z_Construct_UScriptStruct_FConsoleCommand_Statics;
CONSOLECOMMANDPANEL_API UScriptStruct* Z_Construct_UScriptStruct_FConsoleCommand(ETypeConstructPhase);

#define FID_Users_Administrator_Documents_Unreal_Projects_World_Plugins_ConsoleCommandPanel_Source_ConsoleCommandPanel_Public_ConsoleCommandPanelSettings_h_10_GENERATED_BODY \
	friend struct ::Z_Construct_UScriptStruct_FConsoleCommand_Statics; \
	UE_NODEBUG static UScriptStruct* StaticStruct() { return Z_Construct_UScriptStruct_FConsoleCommand(ETypeConstructPhase::Inner); }


struct FConsoleCommand;
// ********** End ScriptStruct FConsoleCommand *****************************************************

// ********** Begin Class UConsoleCommandPanelSettings *********************************************
struct Z_Construct_UClass_UConsoleCommandPanelSettings_Statics;
CONSOLECOMMANDPANEL_API UClass* Z_Construct_UClass_UConsoleCommandPanelSettings(ETypeConstructPhase);

#define FID_Users_Administrator_Documents_Unreal_Projects_World_Plugins_ConsoleCommandPanel_Source_ConsoleCommandPanel_Public_ConsoleCommandPanelSettings_h_21_INCLASS_NO_PURE_DECLS \
private: \
	friend struct ::Z_Construct_UClass_UConsoleCommandPanelSettings_Statics; \
	friend CONSOLECOMMANDPANEL_API UClass* ::Z_Construct_UClass_UConsoleCommandPanelSettings(ETypeConstructPhase); \
public: \
	DECLARE_CLASS2(UConsoleCommandPanelSettings, UDeveloperSettings, COMPILED_IN_FLAGS(0 | CLASS_Config), CASTCLASS_None, TEXT("/Script/ConsoleCommandPanel"), Z_Construct_UClass_UConsoleCommandPanelSettings) \
	DECLARE_SERIALIZER(UConsoleCommandPanelSettings) \
	static constexpr const TCHAR* StaticConfigName() {return TEXT("EditorPerProjectUserSettings");} \



#define FID_Users_Administrator_Documents_Unreal_Projects_World_Plugins_ConsoleCommandPanel_Source_ConsoleCommandPanel_Public_ConsoleCommandPanelSettings_h_21_ENHANCED_CONSTRUCTORS \
	/** Standard constructor, called after all reflected properties have been initialized */ \
	NO_API UConsoleCommandPanelSettings(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get()); \
	/** Deleted move- and copy-constructors, should never be used */ \
	UConsoleCommandPanelSettings(UConsoleCommandPanelSettings&&) = delete; \
	UConsoleCommandPanelSettings(const UConsoleCommandPanelSettings&) = delete; \
	DECLARE_VTABLE_PTR_HELPER_CTOR(NO_API, UConsoleCommandPanelSettings); \
	DEFINE_VTABLE_PTR_HELPER_CTOR_CALLER(UConsoleCommandPanelSettings); \
	DEFINE_DEFAULT_OBJECT_INITIALIZER_CONSTRUCTOR_CALL(UConsoleCommandPanelSettings) \
	NO_API virtual ~UConsoleCommandPanelSettings();


#define FID_Users_Administrator_Documents_Unreal_Projects_World_Plugins_ConsoleCommandPanel_Source_ConsoleCommandPanel_Public_ConsoleCommandPanelSettings_h_18_PROLOG
#define FID_Users_Administrator_Documents_Unreal_Projects_World_Plugins_ConsoleCommandPanel_Source_ConsoleCommandPanel_Public_ConsoleCommandPanelSettings_h_21_GENERATED_BODY \
PRAGMA_DISABLE_DEPRECATION_WARNINGS \
public: \
	FID_Users_Administrator_Documents_Unreal_Projects_World_Plugins_ConsoleCommandPanel_Source_ConsoleCommandPanel_Public_ConsoleCommandPanelSettings_h_21_INCLASS_NO_PURE_DECLS \
	FID_Users_Administrator_Documents_Unreal_Projects_World_Plugins_ConsoleCommandPanel_Source_ConsoleCommandPanel_Public_ConsoleCommandPanelSettings_h_21_ENHANCED_CONSTRUCTORS \
private: \
PRAGMA_ENABLE_DEPRECATION_WARNINGS


class UConsoleCommandPanelSettings;

// ********** End Class UConsoleCommandPanelSettings ***********************************************

#undef CURRENT_FILE_ID
#define CURRENT_FILE_ID FID_Users_Administrator_Documents_Unreal_Projects_World_Plugins_ConsoleCommandPanel_Source_ConsoleCommandPanel_Public_ConsoleCommandPanelSettings_h

PRAGMA_ENABLE_DEPRECATION_WARNINGS
