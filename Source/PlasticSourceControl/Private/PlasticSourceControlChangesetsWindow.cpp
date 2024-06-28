// Copyright Epic Games, Inc. All Rights Reserved.

#include "PlasticSourceControlChangesetsWindow.h"

#include "Framework/Docking/TabManager.h"
#include "Widgets/Docking/SDockTab.h"

#include "PlasticSourceControlStyle.h"
#include "SPlasticSourceControlChangesetsWidget.h"

#define LOCTEXT_NAMESPACE "PlasticSourceControlChangesetsWindow"

static const FName PlasticSourceControlChangesetsWindowTabName("PlasticSourceControlChangesetsWindow");

void FPlasticSourceControlChangesetsWindow::Register()
{
	FPlasticSourceControlStyle::Initialize();
	FPlasticSourceControlStyle::ReloadTextures();

	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(PlasticSourceControlChangesetsWindowTabName, FOnSpawnTab::CreateRaw(this, &FPlasticSourceControlChangesetsWindow::OnSpawnTab))
		.SetDisplayName(LOCTEXT("PlasticSourceControlChangesetsWindowTabTitle", "View Changesets"))
		.SetMenuType(ETabSpawnerMenuType::Hidden)
		.SetIcon(FSlateIcon(FPlasticSourceControlStyle::Get().GetStyleSetName(), "PlasticSourceControl.PluginIcon.Small"));
}

void FPlasticSourceControlChangesetsWindow::Unregister()
{
	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(PlasticSourceControlChangesetsWindowTabName);

	FPlasticSourceControlStyle::Shutdown();
}

TSharedRef<SDockTab> FPlasticSourceControlChangesetsWindow::OnSpawnTab(const FSpawnTabArgs& SpawnTabArgs)
{
	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			CreateChangesetsWidget().ToSharedRef()
		];
}

void FPlasticSourceControlChangesetsWindow::OpenTab()
{
	FGlobalTabmanager::Get()->TryInvokeTab(PlasticSourceControlChangesetsWindowTabName);
}

TSharedPtr<SWidget> FPlasticSourceControlChangesetsWindow::CreateChangesetsWidget()
{
	return SNew(SPlasticSourceControlChangesetsWidget);
}

#undef LOCTEXT_NAMESPACE
