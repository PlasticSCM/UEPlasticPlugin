// Copyright (c) 2023 Unity Technologies

#include "UnityVersionControlWindow.h"

#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"

static const FName UnityVersionControlWindowTabName("UnityVersionControlWindow");

#define LOCTEXT_NAMESPACE "UnityVersionControlWindow"

void FUnityVersionControlWindow::Register()
{
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(UnityVersionControlWindowTabName, FOnSpawnTab::CreateRaw(this, &FUnityVersionControlWindow::OnSpawnPluginTab))
		.SetDisplayName(LOCTEXT("UnityVersionControlWindowTabTitle", "Unity Version Control"))
		.SetMenuType(ETabSpawnerMenuType::Hidden);
}

void FUnityVersionControlWindow::Unregister()
{
	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(UnityVersionControlWindowTabName);
}

TSharedRef<SDockTab> FUnityVersionControlWindow::OnSpawnPluginTab(const FSpawnTabArgs& SpawnTabArgs)
{
	FText WidgetText = FText::Format(
		LOCTEXT("WindowWidgetText", "Add code to {0} in {1} to override this window's contents"),
		FText::FromString(TEXT("FUnityVersionControlWindow::OnSpawnPluginTab")),
		FText::FromString(TEXT("UnityVersionControlWindow.cpp"))
		);

	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			// Put your tab content here!
			SNew(SBox)
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(WidgetText)
			]
		];
}

void FUnityVersionControlWindow::OpenTab()
{
	FGlobalTabmanager::Get()->TryInvokeTab(UnityVersionControlWindowTabName);
}

#undef LOCTEXT_NAMESPACE
