// Copyright (c) 2023 Unity Technologies

#include "UnityVersionControlBranchesWindow.h"

#include "Widgets/Docking/SDockTab.h"

#include "UnityVersionControlStyle.h"
#include "SUnityVersionControlBranchesWidget.h"

#define LOCTEXT_NAMESPACE "UnityVersionControlBranchesWindow"

static const FName UnityVersionControlBranchesWindowTabName("UnityVersionControlBranchesWindow");

void FUnityVersionControlBranchesWindow::Register()
{
	FUnityVersionControlStyle::Initialize();
	FUnityVersionControlStyle::ReloadTextures();

	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(UnityVersionControlBranchesWindowTabName, FOnSpawnTab::CreateRaw(this, &FUnityVersionControlBranchesWindow::OnSpawnTab))
		.SetDisplayName(LOCTEXT("UnityVersionControlBranchesWindowTabTitle", "View Branches"))
		.SetMenuType(ETabSpawnerMenuType::Hidden)
		.SetIcon(FSlateIcon(FUnityVersionControlStyle::Get().GetStyleSetName(), "UnityVersionControl.PluginIcon.Small"));
}

void FUnityVersionControlBranchesWindow::Unregister()
{
	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(UnityVersionControlBranchesWindowTabName);

	FUnityVersionControlStyle::Shutdown();
}

TSharedRef<SDockTab> FUnityVersionControlBranchesWindow::OnSpawnTab(const FSpawnTabArgs& SpawnTabArgs)
{
	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			CreateBranchesWidget().ToSharedRef()
		];
}

void FUnityVersionControlBranchesWindow::OpenTab()
{
	FGlobalTabmanager::Get()->TryInvokeTab(UnityVersionControlBranchesWindowTabName);
}

TSharedPtr<SWidget> FUnityVersionControlBranchesWindow::CreateBranchesWidget()
{
	return SNew(SUnityVersionControlBranchesWidget);
}

#undef LOCTEXT_NAMESPACE
