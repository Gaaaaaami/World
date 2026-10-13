// Copyright 2026 GlimmerWorks Studio. All Rights Reserved.
#pragma once
#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "ConsoleCommandPanelSettings.generated.h"

USTRUCT()
struct FConsoleCommand
{
    GENERATED_BODY()
    UPROPERTY(Config, EditAnywhere, Category = "ConsoleCommand")
    FString DisplayName;

    UPROPERTY(Config, EditAnywhere, Category = "ConsoleCommand")
    FString Command;
};

UCLASS(config = EditorPerProjectUserSettings)
class CONSOLECOMMANDPANEL_API UConsoleCommandPanelSettings : public UDeveloperSettings
{
    GENERATED_BODY()
public:
    UPROPERTY(Config, EditAnywhere, Category = "ConsoleCommandPanel")
    TArray<FConsoleCommand> Commands;

    static UConsoleCommandPanelSettings* Get() { return GetMutableDefault<UConsoleCommandPanelSettings>(); }
};