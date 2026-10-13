// Copyright 2026 GlimmerWorks Studio. All Rights Reserved.
#include "ConsoleCommandPanelCommands.h"

#define LOCTEXT_NAMESPACE "FConsoleCommandPanelModule"

void FConsoleCommandPanelCommands::RegisterCommands()
{
	UI_COMMAND(OpenPluginWindow, "ConsoleCommandPanel", "Bring up ConsoleCommandPanel window", EUserInterfaceActionType::Button, FInputChord());
}

#undef LOCTEXT_NAMESPACE
