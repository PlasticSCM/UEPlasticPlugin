// Copyright (c) 2025 Unity Technologies

#include "UnityVersionControlLocksWindow.h"

#include "Framework/Docking/TabManager.h"
#include "Widgets/Docking/SDockTab.h"

#include "UnityVersionControlStyle.h"
#include "SUnityVersionControlLocksWidget.h"

#define LOCTEXT_NAMESPACE "UnityVersionControlLocksWindow"

static const FName UnityVersionControlLocksWindowTabName("UnityVersionControlLocksWindow");

void FUnityVersionControlLocksWindow::Register()
{
	FUnityVersionControlStyle::Initialize();
	FUnityVersionControlStyle::ReloadTextures();

	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(UnityVersionControlLocksWindowTabName, FOnSpawnTab::CreateRaw(this, &FUnityVersionControlLocksWindow::OnSpawnTab))
		.SetDisplayName(LOCTEXT("UnityVersionControlLocksWindowTabTitle", "View Locks"))
		.SetMenuType(ETabSpawnerMenuType::Hidden)
		.SetIcon(FSlateIcon(FUnityVersionControlStyle::Get().GetStyleSetName(), "UnityVersionControl.PluginIcon.Small"));
}

void FUnityVersionControlLocksWindow::Unregister()
{
	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(UnityVersionControlLocksWindowTabName);

	FUnityVersionControlStyle::Shutdown();
}

TSharedRef<SDockTab> FUnityVersionControlLocksWindow::OnSpawnTab(const FSpawnTabArgs& SpawnTabArgs)
{
	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			CreateLocksWidget().ToSharedRef()
		];
}

void FUnityVersionControlLocksWindow::OpenTab()
{
	FGlobalTabmanager::Get()->TryInvokeTab(UnityVersionControlLocksWindowTabName);
}

TSharedPtr<SWidget> FUnityVersionControlLocksWindow::CreateLocksWidget()
{
	return SNew(SUnityVersionControlLocksWidget);
}

#undef LOCTEXT_NAMESPACE
