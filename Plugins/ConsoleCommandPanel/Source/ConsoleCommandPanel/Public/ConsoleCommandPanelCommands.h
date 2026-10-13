// Copyright 2026 GlimmerWorks Studio. All Rights Reserved.
#pragma once

#include "Framework/Commands/Commands.h"
#include "ConsoleCommandPanelStyle.h"

class FConsoleCommandPanelCommands : public TCommands<FConsoleCommandPanelCommands>
{
public:

	FConsoleCommandPanelCommands()
		: TCommands<FConsoleCommandPanelCommands>(TEXT("ConsoleCommandPanel"), NSLOCTEXT("Contexts", "ConsoleCommandPanel", "ConsoleCommandPanel Plugin"), NAME_None, FConsoleCommandPanelStyle::GetStyleSetName())
	{
	}

	// TCommands<> interface
	virtual void RegisterCommands() override;

public:
	TSharedPtr< FUICommandInfo > OpenPluginWindow;
};