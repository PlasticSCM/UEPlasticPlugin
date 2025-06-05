// Copyright (c) 2025 Unity Technologies

#include "UnityVersionControlChangesetsWindow.h"

#include "Framework/Docking/TabManager.h"
#include "Widgets/Docking/SDockTab.h"

#include "UnityVersionControlStyle.h"
#include "SUnityVersionControlChangesetsWidget.h"

#define LOCTEXT_NAMESPACE "UnityVersionControlChangesetsWindow"

static const FName UnityVersionControlChangesetsWindowTabName("UnityVersionControlChangesetsWindow");

void FUnityVersionControlChangesetsWindow::Register()
{
	FUnityVersionControlStyle::Initialize();
	FUnityVersionControlStyle::ReloadTextures();

	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(UnityVersionControlChangesetsWindowTabName, FOnSpawnTab::CreateRaw(this, &FUnityVersionControlChangesetsWindow::OnSpawnTab))
		.SetDisplayName(LOCTEXT("UnityVersionControlChangesetsWindowTabTitle", "View Changesets"))
		.SetMenuType(ETabSpawnerMenuType::Hidden)
		.SetIcon(FSlateIcon(FUnityVersionControlStyle::Get().GetStyleSetName(), "UnityVersionControl.PluginIcon.Small"));
}

void FUnityVersionControlChangesetsWindow::Unregister()
{
	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(UnityVersionControlChangesetsWindowTabName);

	FUnityVersionControlStyle::Shutdown();
}

TSharedRef<SDockTab> FUnityVersionControlChangesetsWindow::OnSpawnTab(const FSpawnTabArgs& SpawnTabArgs)
{
	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			CreateChangesetsWidget().ToSharedRef()
		];
}

void FUnityVersionControlChangesetsWindow::OpenTab()
{
	FGlobalTabmanager::Get()->TryInvokeTab(UnityVersionControlChangesetsWindowTabName);
}

TSharedPtr<SWidget> FUnityVersionControlChangesetsWindow::CreateChangesetsWidget()
{
	return SNew(SUnityVersionControlChangesetsWidget);
}

#undef LOCTEXT_NAMESPACE
