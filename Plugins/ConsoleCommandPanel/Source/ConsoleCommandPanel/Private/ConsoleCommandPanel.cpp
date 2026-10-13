// Copyright 2026 GlimmerWorks Studio. All Rights Reserved.
#include "ConsoleCommandPanel.h"
#include "ConsoleCommandPanelStyle.h"
#include "ConsoleCommandPanelCommands.h"
#include "ConsoleCommandPanelSettings.h"
#include "LevelEditor.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SSuggestionTextBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Views/SListView.h"
#include "Editor.h"
#include "ToolMenus.h"
#include "Toolkits/FConsoleCommandExecutor.h"
#include "HAL/IConsoleManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Interfaces/IPluginManager.h"
#include "UObject/UnrealType.h"

static const FName ConsoleCommandPanelTabName("ConsoleCommandPanel");

#define LOCTEXT_NAMESPACE "FConsoleCommandPanelModule"

namespace
{
    void GetConsoleSuggestions(const FString& Input, TArray<FString>& OutSuggestions)
    {
        FConsoleCommandExecutor Executor;
        TArray<FConsoleSuggestion> Suggestions;
        Executor.GetSuggestedCompletions(*Input, Suggestions);

        for (const FConsoleSuggestion& S : Suggestions)
        {
            OutSuggestions.Add(S.Name);  // 改为 S.Name
        }
    }
}
void FConsoleCommandPanelModule::LoadDefaultCommands()
{
    if (!UObjectInitialized() || IsEngineExitRequested()) return;
    UConsoleCommandPanelSettings* Settings = GetMutableDefault<UConsoleCommandPanelSettings>();
    if (!Settings) return;

    // 用户已保存过配置就不覆盖
    if (Settings->Commands.Num() > 0) return;

    TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("ConsoleCommandPanel"));
    if (!Plugin.IsValid()) return;

    const FString IniPath = FPaths::Combine(
        Plugin->GetBaseDir(), TEXT("Config/DefaultConsoleCommandPanel.ini"));

    TArray<FString> Lines;
    if (!FFileHelper::LoadFileToStringArray(Lines, *IniPath)) return;

    UScriptStruct* Struct = FConsoleCommand::StaticStruct();

    for (FString Line : Lines)
    {
        Line.TrimStartAndEndInline();
        if (!Line.StartsWith(TEXT("+Commands="))) continue;

        FString Payload = Line.RightChop(10); // 去掉 "+Commands="

        FConsoleCommand Cmd;
        const TCHAR* Result = Struct->ImportText(
            *Payload, &Cmd, nullptr, PPF_None, nullptr, Struct->GetName());

        if (Result)
        {
            Settings->Commands.Add(Cmd);
        }
    }

}
void FConsoleCommandPanelModule::StartupModule()
{
    FConsoleCommandPanelStyle::Initialize();
    FConsoleCommandPanelStyle::ReloadTextures();

    FConsoleCommandPanelCommands::Register();

    PluginCommands = MakeShareable(new FUICommandList);

    PluginCommands->MapAction(
        FConsoleCommandPanelCommands::Get().OpenPluginWindow,
        FExecuteAction::CreateRaw(this, &FConsoleCommandPanelModule::PluginButtonClicked),
        FCanExecuteAction());

    UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FConsoleCommandPanelModule::RegisterMenus));

    FGlobalTabmanager::Get()->RegisterNomadTabSpawner(ConsoleCommandPanelTabName, FOnSpawnTab::CreateRaw(this, &FConsoleCommandPanelModule::OnSpawnPluginTab))
        .SetDisplayName(LOCTEXT("FConsoleCommandPanelTabTitle", "ConsoleCommandPanel"))
        .SetMenuType(ETabSpawnerMenuType::Hidden);

    FCoreDelegates::OnPostEngineInit.AddRaw(this, &FConsoleCommandPanelModule::LoadDefaultCommands);
}

void FConsoleCommandPanelModule::ShutdownModule()
{
    FCoreDelegates::OnPostEngineInit.RemoveAll(this);
    UToolMenus::UnRegisterStartupCallback(this);
    UToolMenus::UnregisterOwner(this);
    FConsoleCommandPanelStyle::Shutdown();
    FConsoleCommandPanelCommands::Unregister();
    FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(ConsoleCommandPanelTabName);
}



TSharedRef<SDockTab> FConsoleCommandPanelModule::OnSpawnPluginTab(const FSpawnTabArgs& SpawnTabArgs)
{
    UConsoleCommandPanelSettings* Settings = UConsoleCommandPanelSettings::Get();

    TSharedPtr<TArray<TSharedPtr<FConsoleCommand>>> FullList = MakeShareable(new TArray<TSharedPtr<FConsoleCommand>>());
    TSharedPtr<TArray<TSharedPtr<FConsoleCommand>>> VisibleList = MakeShareable(new TArray<TSharedPtr<FConsoleCommand>>());
    for (const FConsoleCommand& Cmd : Settings->Commands)
    {
        FullList->Add(MakeShareable(new FConsoleCommand(Cmd)));
    }
    *VisibleList = *FullList;

    TSharedPtr<FString> CurrentCommandText = MakeShareable(new FString());
    TSharedPtr<FText> ActionButtonText = MakeShareable(new FText(FText::FromString(TEXT("Add"))));
    TSharedPtr<TSharedPtr<FConsoleCommand>> EditingItem = MakeShareable(new TSharedPtr<FConsoleCommand>());
    TSharedPtr<TSharedPtr<SListView<TSharedPtr<FConsoleCommand>>>> ListViewHolder =
        MakeShareable(new TSharedPtr<SListView<TSharedPtr<FConsoleCommand>>>());
    TSharedPtr<TSharedPtr<SButton>> ActionButtonHolder = MakeShareable(new TSharedPtr<SButton>());
    TSharedPtr<SEditableTextBox> SearchInput;

    // ===== Tooltip builder =====
    auto BuildTooltip = [](const FString& FullCommand) -> FString
        {
            if (FullCommand.IsEmpty()) return FString();

            FString CommandName = FullCommand;
            FString UserParams;

            // 先按完整字符串查（有些命令注册时带参数段）
            IConsoleObject* Obj = IConsoleManager::Get().FindConsoleObject(*FullCommand);

            // 找不到再按第一个空格拆分
            if (!Obj)
            {
                int32 SpaceIdx;
                if (FullCommand.FindChar(TEXT(' '), SpaceIdx))
                {
                    CommandName = FullCommand.Left(SpaceIdx);
                    UserParams = FullCommand.Mid(SpaceIdx + 1).TrimStartAndEnd();
                    Obj = IConsoleManager::Get().FindConsoleObject(*CommandName);
                }
            }

            // 都找不到：不显示 "(command not found)"，只显示命令本身
            // 因为 stat、show、r. 等系列命令的注册方式不一定走 IConsoleManager
            if (!Obj)
            {
                return FString::Printf(TEXT("[%s]"), *FullCommand);
            }

            FString Help = Obj->GetHelp();
            FString Description;
            FString ParamHelp;
            int32 BracketIdx = INDEX_NONE;
            for (int32 i = 0; i < Help.Len(); ++i)
            {
                if (Help[i] == TEXT('[') || Help[i] == TEXT('{')) { BracketIdx = i; break; }
            }
            if (BracketIdx == INDEX_NONE) Description = Help.TrimStartAndEnd();
            else if (BracketIdx == 0)     ParamHelp = Help.TrimStartAndEnd();
            else
            {
                Description = Help.Left(BracketIdx).TrimStartAndEnd();
                ParamHelp = Help.Mid(BracketIdx).TrimStartAndEnd();
            }

            FString Result = FString::Printf(TEXT("[%s | %s]"),
                *CommandName, UserParams.IsEmpty() ? TEXT("-") : *UserParams);
            if (!Description.IsEmpty()) Result += TEXT("\n") + Description;
            if (!ParamHelp.IsEmpty())   Result += TEXT("\n") + ParamHelp;
            return Result;
        };
    // ===== Command input factory =====
    auto MakeCommandInput = [CurrentCommandText]() -> TSharedRef<SWidget>
        {
            return SNew(SSuggestionTextBox)
                .Text(FText::FromString(*CurrentCommandText))
                .HintText(FText::FromString(TEXT("Console Command")))
                .ForegroundColor(FSlateColor(FLinearColor::White))
                .OnTextChanged_Lambda([CurrentCommandText](const FText& NewText)
                    {
                        *CurrentCommandText = NewText.ToString();
                    })
                .OnShowingSuggestions_Lambda([](const FString& Input, TArray<FString>& Out)
                    {
                        GetConsoleSuggestions(Input, Out);
                    });
        };

    // ===== Pre-build input widgets so lambdas capture valid pointers =====
    TSharedPtr<SEditableTextBox> DisplayNameInput = SNew(SEditableTextBox)
        .HintText(FText::FromString(TEXT("Display Name")));

    TSharedPtr<SBox> CommandInputBox = SNew(SBox)
        .ToolTipText_Lambda([CurrentCommandText, BuildTooltip]() -> FText
            {
                FString Cmd = CurrentCommandText->TrimStartAndEnd();
                if (Cmd.IsEmpty()) return FText::GetEmpty();
                return FText::FromString(BuildTooltip(Cmd));
            })
        [
            MakeCommandInput()
        ];

    // ===== Build ListView =====
    TSharedRef<SListView<TSharedPtr<FConsoleCommand>>> ListViewRef =
        SNew(SListView<TSharedPtr<FConsoleCommand>>)
        .ListItemsSource(VisibleList.Get())
        .OnGenerateRow_Lambda([=](TSharedPtr<FConsoleCommand> Item, const TSharedRef<STableViewBase>& OwnerTable)
            {
                if (!Item.IsValid())
                    return SNew(STableRow<TSharedPtr<FConsoleCommand>>, OwnerTable);

                return SNew(STableRow<TSharedPtr<FConsoleCommand>>, OwnerTable)
                    .ToolTipText_Lambda([Item, BuildTooltip]() -> FText
                        {
                            return FText::FromString(BuildTooltip(Item->Command));
                        })
                    [
                        SNew(SHorizontalBox)
                            + SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center).Padding(5)
                            [
                                SNew(STextBlock).Text(FText::FromString(Item->DisplayName))
                            ]
                            + SHorizontalBox::Slot().AutoWidth().Padding(2)
                            [
                                SNew(SButton).Text(FText::FromString(TEXT("Run")))
                                    .OnClicked_Lambda([Item]()
                                        {
                                            if (GEditor) GEditor->Exec(GEditor->GetEditorWorldContext().World(), *Item->Command);
                                            return FReply::Handled();
                                        })
                            ]
                        + SHorizontalBox::Slot().AutoWidth().Padding(2)
                            [
                                SNew(SButton).Text(FText::FromString(TEXT("Edit")))
                                    .OnClicked_Lambda([=]()
                                        {
                                            *EditingItem = Item;
                                            *CurrentCommandText = Item->Command;

                                            DisplayNameInput->SetText(FText::FromString(Item->DisplayName));
                                            CommandInputBox->SetContent(MakeCommandInput());

                                            *ActionButtonText = FText::FromString(TEXT("Update"));
                                            if (ActionButtonHolder->IsValid())
                                            {
                                                (*ActionButtonHolder)->Invalidate(EInvalidateWidgetReason::Layout);
                                            }
                                            return FReply::Handled();
                                        })
                            ]
                        + SHorizontalBox::Slot().AutoWidth().Padding(2)
                            [
                                SNew(SButton).Text(FText::FromString(TEXT("Delete")))
                                    .OnClicked_Lambda([=]()
                                        {
                                            Settings->Commands.RemoveAll([Item](const FConsoleCommand& C)
                                                {
                                                    return C.DisplayName == Item->DisplayName && C.Command == Item->Command;
                                                });
                                            Settings->SaveConfig();

                                            FullList->RemoveSingle(Item);
                                            VisibleList->RemoveSingle(Item);

                                            if ((*EditingItem).IsValid() && (*EditingItem) == Item)
                                            {
                                                (*EditingItem).Reset();
                                            }

                                            if (ListViewHolder->IsValid()) (*ListViewHolder)->RequestListRefresh();
                                            return FReply::Handled();
                                        })
                            ]
                    ];
            });

    *ListViewHolder = ListViewRef;

    // ===== Tab =====
    TSharedRef<SDockTab> Tab = SNew(SDockTab)
        .TabRole(ETabRole::NomadTab)
        [
            SNew(SVerticalBox)

                // ===== Input area =====
                + SVerticalBox::Slot().AutoHeight().Padding(5)
                [
                    SNew(SHorizontalBox)
                        + SHorizontalBox::Slot().FillWidth(0.3f).Padding(2)
                        [
                            DisplayNameInput.ToSharedRef()
                        ]
                        + SHorizontalBox::Slot().FillWidth(0.7f).Padding(2)
                        [
                            CommandInputBox.ToSharedRef()
                        ]
                        + SHorizontalBox::Slot().AutoWidth().Padding(2)
                        [
                            SAssignNew(*ActionButtonHolder, SButton)
                                .Text_Lambda([ActionButtonText]() { return *ActionButtonText; })
                                .OnClicked_Lambda([=]()
                                    {
                                        FString Name = DisplayNameInput->GetText().ToString();
                                        FString Cmd = *CurrentCommandText;
                                        if (Name.IsEmpty() || Cmd.IsEmpty()) return FReply::Handled();

                                        if ((*EditingItem).IsValid())
                                        {
                                            TSharedPtr<FConsoleCommand> Old = *EditingItem;
                                            for (FConsoleCommand& S : Settings->Commands)
                                            {
                                                if (S.DisplayName == Old->DisplayName && S.Command == Old->Command)
                                                {
                                                    S.DisplayName = Name;
                                                    S.Command = Cmd;
                                                    break;
                                                }
                                            }
                                            Old->DisplayName = Name;
                                            Old->Command = Cmd;
                                            (*EditingItem).Reset();
                                        }
                                        else
                                        {
                                            FConsoleCommand NewCmd;
                                            NewCmd.DisplayName = Name;
                                            NewCmd.Command = Cmd;
                                            Settings->Commands.Add(NewCmd);

                                            TSharedPtr<FConsoleCommand> NewItem = MakeShareable(new FConsoleCommand(NewCmd));
                                            FullList->Add(NewItem);
                                            VisibleList->Add(NewItem);
                                        }

                                        Settings->SaveConfig();

                                        DisplayNameInput->SetText(FText::GetEmpty());
                                        *CurrentCommandText = TEXT("");
                                        CommandInputBox->SetContent(MakeCommandInput());

                                        *ActionButtonText = FText::FromString(TEXT("Add"));
                                        if (ActionButtonHolder->IsValid())
                                        {
                                            (*ActionButtonHolder)->Invalidate(EInvalidateWidgetReason::Layout);
                                        }

                                        if (ListViewHolder->IsValid()) (*ListViewHolder)->RequestListRefresh();
                                        return FReply::Handled();
                                    })
                        ]
                ]

            // ===== Search area =====
            + SVerticalBox::Slot().AutoHeight().Padding(5, 0, 5, 5)
                [
                    SAssignNew(SearchInput, SEditableTextBox)
                        .HintText(FText::FromString(TEXT("Search...")))
                        .OnTextChanged_Lambda([=](const FText& NewText)
                            {
                                FString Query = NewText.ToString().TrimStartAndEnd();
                                VisibleList->Empty();

                                if (Query.IsEmpty())
                                {
                                    *VisibleList = *FullList;
                                }
                                else
                                {
                                    for (const TSharedPtr<FConsoleCommand>& Item : *FullList)
                                    {
                                        if (!Item.IsValid()) continue;
                                        if (Item->DisplayName.Contains(Query, ESearchCase::IgnoreCase) ||
                                            Item->Command.Contains(Query, ESearchCase::IgnoreCase))
                                        {
                                            VisibleList->Add(Item);
                                        }
                                    }
                                }

                                if (ListViewHolder->IsValid()) (*ListViewHolder)->RequestListRefresh();
                            })
                ]

            // ===== List area =====
            + SVerticalBox::Slot().FillHeight(1.0f).Padding(5)
                [
                    ListViewRef
                ]
        ];

    return Tab;
}

void FConsoleCommandPanelModule::PluginButtonClicked()
{
    FGlobalTabmanager::Get()->TryInvokeTab(ConsoleCommandPanelTabName);
}

void FConsoleCommandPanelModule::RegisterMenus()
{
    FToolMenuOwnerScoped OwnerScoped(this);

    {
        UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Window");
        {
            FToolMenuSection& Section = Menu->FindOrAddSection("WindowLayout");
            Section.AddMenuEntryWithCommandList(FConsoleCommandPanelCommands::Get().OpenPluginWindow, PluginCommands);
        }
    }

    {
        UToolMenu* ToolbarMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.LevelEditorToolBar.PlayToolBar");
        {
            FToolMenuSection& Section = ToolbarMenu->FindOrAddSection("PluginTools");
            {
                FToolMenuEntry& Entry = Section.AddEntry(FToolMenuEntry::InitToolBarButton(FConsoleCommandPanelCommands::Get().OpenPluginWindow));
                Entry.SetCommandList(PluginCommands);
            }
        }
    }
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FConsoleCommandPanelModule, ConsoleCommandPanel)