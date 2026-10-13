// Copyright 2026 GlimmerWorks Studio. All Rights Reserved.
#include "ConsoleCommandPanelStyle.h"
#include "Styling/SlateStyleRegistry.h"
#include "Framework/Application/SlateApplication.h"
#include "Slate/SlateGameResources.h"
#include "Interfaces/IPluginManager.h"
#include "Styling/SlateStyleMacros.h"

#define RootToContentDir Style->RootToContentDir

TSharedPtr<FSlateStyleSet> FConsoleCommandPanelStyle::StyleInstance = nullptr;

void FConsoleCommandPanelStyle::Initialize()
{
	if (!StyleInstance.IsValid())
	{
		StyleInstance = Create();
		FSlateStyleRegistry::RegisterSlateStyle(*StyleInstance);
	}
}

void FConsoleCommandPanelStyle::Shutdown()
{
	FSlateStyleRegistry::UnRegisterSlateStyle(*StyleInstance);
	ensure(StyleInstance.IsUnique());
	StyleInstance.Reset();
}

FName FConsoleCommandPanelStyle::GetStyleSetName()
{
	static FName StyleSetName(TEXT("ConsoleCommandPanelStyle"));
	return StyleSetName;
}

const FVector2D Icon16x16(16.0f, 16.0f);
const FVector2D Icon20x20(20.0f, 20.0f);

TSharedRef< FSlateStyleSet > FConsoleCommandPanelStyle::Create()
{
	TSharedRef< FSlateStyleSet > Style = MakeShareable(new FSlateStyleSet("ConsoleCommandPanelStyle"));
	Style->SetContentRoot(IPluginManager::Get().FindPlugin("ConsoleCommandPanel")->GetBaseDir() / TEXT("Resources"));

	Style->Set("ConsoleCommandPanel.OpenPluginWindow", new IMAGE_BRUSH_SVG(TEXT("PlaceholderButtonIcon"), Icon20x20));
	Style->Set("ConsoleCommandPanel.OpenPluginWindow",
		new IMAGE_BRUSH_SVG(TEXT("ConsoleIcon"), Icon20x20));

	return Style;
}

void FConsoleCommandPanelStyle::ReloadTextures()
{
	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().GetRenderer()->ReloadTextureResources();
	}
}

const ISlateStyle& FConsoleCommandPanelStyle::Get()
{
	return *StyleInstance;
}
