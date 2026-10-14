// Copyright 2026 GlimmerWorks Studio. All Rights Reserved.

#pragma once
#include "Modules/ModuleManager.h"

class FToolBarBuilder;
class FMenuBuilder;

class FConsoleCommandPanelModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	
	/** This function will be bound to Command (by default it will bring up plugin window) */
	void PluginButtonClicked();
	void LoadDefaultCommands();
private:

	void RegisterMenus();

	TSharedRef<class SDockTab> OnSpawnPluginTab(const class FSpawnTabArgs& SpawnTabArgs);

private:
	TSharedPtr<class FUICommandList> PluginCommands;

private:
	TSharedPtr<class SListView<TSharedPtr<struct FConsoleCommand>>> CommandListView;

};
