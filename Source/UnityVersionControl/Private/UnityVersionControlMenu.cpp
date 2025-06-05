// Copyright (c) 2025 Unity Technologies

#include "UnityVersionControlMenu.h"

#include "UnityVersionControlLock.h"
#include "UnityVersionControlModule.h"
#include "UnityVersionControlOperations.h"
#include "UnityVersionControlProvider.h"
#include "UnityVersionControlStyle.h"
#include "UnityVersionControlUtils.h"
#include "UnityVersionControlVersions.h"
#include "SUnityVersionControlStatusBar.h"

#include "ISourceControlModule.h"
#include "ISourceControlOperation.h"
#include "SourceControlOperations.h"

#include "ContentBrowserMenuContexts.h"
#include "Interfaces/IPluginManager.h"
#include "HAL/PlatformProcess.h"
#include "LevelEditor.h"
#include "Misc/MessageDialog.h"
#include "Modules/ModuleManager.h"
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
#include "Styling/AppStyle.h"
#else
#include "EditorStyleSet.h"
#endif

#include "PackageUtils.h"
#include "ISettingsModule.h"

#include "Logging/MessageLog.h"

#include "ToolMenus.h"
#include "ToolMenuMisc.h"

#define LOCTEXT_NAMESPACE "UnityVersionControl"

FName FUnityVersionControlMenu::UnityVersionControlMainMenuOwnerName = TEXT("UnityVersionControlMenu");
FName FUnityVersionControlMenu::UnityVersionControlAssetContextLocksMenuOwnerName = TEXT("UnityVersionControlContextLocksMenu");
FName FUnityVersionControlMenu::UnityVersionControlStatusBarMenuOwnerName = TEXT("UnityVersionControlStatusBarMenu");

void FUnityVersionControlMenu::Register()
{
	if (bHasRegistered)
	{
		return;
	}

	// Register the menu extensions with the level editor
	ExtendRevisionControlMenu();
	ExtendAssetContextMenu();

	ExtendToolbarWithStatusBarWidget();
}

void FUnityVersionControlMenu::Unregister()
{
	if (!bHasRegistered)
	{
		return;
	}

	// Unregister the menu extensions from the level editor
#if ENGINE_MAJOR_VERSION == 4
	if (FLevelEditorModule* LevelEditorModule = FModuleManager::GetModulePtr<FLevelEditorModule>("LevelEditor"))
	{
		LevelEditorModule->GetAllLevelEditorToolbarSourceControlMenuExtenders().RemoveAll([=](const FLevelEditorModule::FLevelEditorMenuExtender& Extender) { return Extender.GetHandle() == ViewMenuExtenderHandle; });
		bHasRegistered = false;
	}
	if (UToolMenus* ToolMenus = UToolMenus::TryGet())
	{
		ToolMenus->UnregisterOwnerByName(UnityVersionControlAssetContextLocksMenuOwnerName);
	}
#elif ENGINE_MAJOR_VERSION == 5
	if (UToolMenus* ToolMenus = UToolMenus::TryGet())
	{
		ToolMenus->UnregisterOwnerByName(UnityVersionControlMainMenuOwnerName);
		ToolMenus->UnregisterOwnerByName(UnityVersionControlAssetContextLocksMenuOwnerName);
		ToolMenus->UnregisterOwnerByName(UnityVersionControlStatusBarMenuOwnerName);
		bHasRegistered = false;
	}
#endif
}

void FUnityVersionControlMenu::ExtendToolbarWithStatusBarWidget()
{
#if ENGINE_MAJOR_VERSION == 5
	const FToolMenuOwnerScoped SourceControlMenuOwner(UnityVersionControlStatusBarMenuOwnerName);

	UToolMenu* ToolbarMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.StatusBar.ToolBar");
	FToolMenuSection& Section = ToolbarMenu->AddSection("Unity Version Control", FText::GetEmpty(), FToolMenuInsert("SourceControl", EToolMenuInsertType::Before));

	Section.AddEntry(
		FToolMenuEntry::InitWidget("UnityVersionControlStatusBar", SNew(SUnityVersionControlStatusBar), FText::GetEmpty(), true, false)
	);
#endif
}

void FUnityVersionControlMenu::ExtendRevisionControlMenu()
{
#if ENGINE_MAJOR_VERSION == 4
	if (FLevelEditorModule* LevelEditorModule = FModuleManager::GetModulePtr<FLevelEditorModule>("LevelEditor"))
	{
		FLevelEditorModule::FLevelEditorMenuExtender ViewMenuExtender = FLevelEditorModule::FLevelEditorMenuExtender::CreateRaw(this, &FUnityVersionControlMenu::OnExtendLevelEditorViewMenu);
		auto& MenuExtenders = LevelEditorModule->GetAllLevelEditorToolbarSourceControlMenuExtenders();
		MenuExtenders.Add(ViewMenuExtender);
		ViewMenuExtenderHandle = MenuExtenders.Last().GetHandle();

		bHasRegistered = true;
	}
#elif ENGINE_MAJOR_VERSION == 5
	const FToolMenuOwnerScoped SourceControlMenuOwner(UnityVersionControlMainMenuOwnerName);

	if (UToolMenu* SourceControlMenu = UToolMenus::Get()->ExtendMenu("StatusBar.ToolBar.SourceControl"))
	{
		FToolMenuSection& Section = SourceControlMenu->AddSection("UnityVersionControlActions", LOCTEXT("UnityVersionControlMenuHeadingActions", "Unity Version Control"), FToolMenuInsert(NAME_None, EToolMenuInsertType::First));

		AddMenuExtension(Section);

		bHasRegistered = true;
	}

	if (UToolMenu* ToolsMenu = UToolMenus::Get()->ExtendMenu("MainFrame.MainMenu.Tools"))
	{
		if (FToolMenuSection* Section = ToolsMenu->FindSection("Source Control"))
		{
			AddViewBranches(*Section);
			AddViewChangesets(*Section);
			AddViewLocks(*Section);
		}
	}
#endif
}

void FUnityVersionControlMenu::ExtendAssetContextMenu()
{
	const FToolMenuOwnerScoped SourceControlMenuOwner(UnityVersionControlAssetContextLocksMenuOwnerName);
	if (UToolMenu* const Menu = UToolMenus::Get()->ExtendMenu(TEXT("ContentBrowser.AssetContextMenu")))
	{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 2
		FToolMenuSection& Section = Menu->AddSection(TEXT("PlasticAssetContextLocksMenuSection"), FText::GetEmpty(), FToolMenuInsert("AssetContextReferences", EToolMenuInsertType::After));
#else
		FToolMenuSection& Section = Menu->AddSection(TEXT("PlasticAssetContextLocksMenuSection"), FText::GetEmpty(), FToolMenuInsert("AssetContextSourceControl", EToolMenuInsertType::Before));
#endif
		Section.AddDynamicEntry(TEXT("PlasticActions"), FNewToolMenuSectionDelegate::CreateLambda([this](FToolMenuSection& InSection)
		{
			UContentBrowserAssetContextMenuContext* Context = InSection.FindContext<UContentBrowserAssetContextMenuContext>();

#if ENGINE_MAJOR_VERSION < 5 || ENGINE_MINOR_VERSION < 1
			TArray<UObject*> SelectedObjects = Context->GetSelectedObjects();
			if (!Context || !Context->bCanBeModified || Context->SelectedObjects.Num() == 0 || !ensure(FUnityVersionControlModule::IsLoaded()))
			{
				return;
			}
			TArray<FAssetData> AssetObjectPaths;
			AssetObjectPaths.Reserve(Context->SelectedObjects.Num());
			for (const auto SelectedObject : SelectedObjects)
			{
				AssetObjectPaths.Add(FAssetData(SelectedObject));
			}
#else
			if (!Context || !Context->bCanBeModified || Context->SelectedAssets.Num() == 0 || !ensure(FUnityVersionControlModule::IsLoaded()))
			{
				return;
			}
			TArray<FAssetData> AssetObjectPaths = Context->SelectedAssets;
#endif

			InSection.AddSubMenu(
				TEXT("PlasticActionsSubMenu"),
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 2
				LOCTEXT("Plastic_ContextMenu", "Revision Control Locks"),
#else
				LOCTEXT("Plastic_ContextMenu", "Source Control Locks"),
#endif
				FText::GetEmpty(),
				FNewMenuDelegate::CreateRaw(this, &FUnityVersionControlMenu::GeneratePlasticAssetContextMenu, MoveTemp(AssetObjectPaths)),
				false,
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
				FSlateIcon(FAppStyle::GetAppStyleSetName(), "PropertyWindow.Locked")
#else
				FSlateIcon(FEditorStyle::GetStyleSetName(), "PropertyWindow.Locked")
#endif
			);
		}));
	}
}

void FUnityVersionControlMenu::GeneratePlasticAssetContextMenu(FMenuBuilder& MenuBuilder, TArray<FAssetData> InAssetObjectPaths)
{
	const FUnityVersionControlProvider& Provider = FUnityVersionControlModule::Get().GetProvider();
	const TArray<FString> Files = PackageUtils::AssetDataToFileNames(InAssetObjectPaths);
	const TArray<FUnityVersionControlLockRef> SelectedLocks = UnityVersionControlUtils::GetLocksForWorkingBranch(Provider, Files);

	MenuBuilder.BeginSection("AssetPlasticActions", LOCTEXT("UnityVersionControlAssetContextLocksMenuHeading", "Unity Version Control Locks"));

	{
		MenuBuilder.AddMenuEntry(
			LOCTEXT("PlasticReleaseLock", "Release Lock"),
			LOCTEXT("PlasticReleaseLockTooltip", "Release Lock(s) on the selected assets.\nReleasing locks will allow other users to keep working on these files and retrieve locks (on the same branch, in the latest revision)."),
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "PropertyWindow.Unlocked"),
#else
			FSlateIcon(FEditorStyle::GetStyleSetName(), "PropertyWindow.Unlocked"),
#endif
			FUIAction(
				FExecuteAction::CreateRaw(this, &FUnityVersionControlMenu::ExecuteReleaseLocks, SelectedLocks),
				FCanExecuteAction::CreateRaw(this, &FUnityVersionControlMenu::CanReleaseLocks, SelectedLocks)
			)
		);
	}

	{
		MenuBuilder.AddMenuEntry(
			LOCTEXT("PlasticRemoveLock", "Remove Lock"),
			LOCTEXT("PlasticRemoveLockTooltip", "Remove Lock(s) on the selected assets.\nRemoving locks will allow other users to edit these files anywhere (on any branch) increasing the risk of future merge conflicts."),
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "PropertyWindow.Unlocked"),
#else
			FSlateIcon(FEditorStyle::GetStyleSetName(), "PropertyWindow.Unlocked"),
#endif
			FUIAction(
				FExecuteAction::CreateRaw(this, &FUnityVersionControlMenu::ExecuteRemoveLocks, SelectedLocks),
				FCanExecuteAction::CreateRaw(this, &FUnityVersionControlMenu::CanRemoveLocks, SelectedLocks)
			)
		);
	}

	FString OrganizationName = FUnityVersionControlModule::Get().GetProvider().GetCloudOrganization();
	if (!OrganizationName.IsEmpty())
	{
		MenuBuilder.AddMenuEntry(
			LOCTEXT("PlasticLockRulesURL", "Configure Lock Rules"),
			LOCTEXT("PlasticLockRulesURLTooltip", "Navigate to lock rules configuration page in the Unity Dashboard."),
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "PropertyWindow.Locked"),
#else
			FSlateIcon(FEditorStyle::GetStyleSetName(), "PropertyWindow.Locked"),
#endif
			FUIAction(
				FExecuteAction::CreateRaw(this, &FUnityVersionControlMenu::VisitLockRulesURLClicked, MoveTemp(OrganizationName)),
				FCanExecuteAction()
			)
		);
	}

	MenuBuilder.EndSection();
}

bool FUnityVersionControlMenu::CanReleaseLocks(TArray<FUnityVersionControlLockRef> InSelectedLocks) const
{
	for (const FUnityVersionControlLockRef& Lock : InSelectedLocks)
	{
		// If "Locked" (currently exclusively Checked Out) the lock can be Released, coming back to it's potential underlying "Retained" status if changes where already checked in the branch
		if (Lock->bIsLocked)
		{
			return true;
		}
	}

	return false;
}

bool FUnityVersionControlMenu::CanRemoveLocks(TArray<FUnityVersionControlLockRef> InSelectedLocks) const
{
	// All "Locked" or "Retained" locks can be Removed
	return (InSelectedLocks.Num() > 0);
}

void FUnityVersionControlMenu::ExecuteReleaseLocks(TArray<FUnityVersionControlLockRef> InSelectedLocks)
{
	ExecuteUnlock(MoveTemp(InSelectedLocks), false);
}

void FUnityVersionControlMenu::ExecuteRemoveLocks(TArray<FUnityVersionControlLockRef> InSelectedLocks)
{
	ExecuteUnlock(MoveTemp(InSelectedLocks), true);
}

void FUnityVersionControlMenu::ExecuteUnlock(TArray<FUnityVersionControlLockRef>&& InSelectedLocks, const bool bInRemove)
{
	if (!Notification.IsInProgress())
	{
		// Launch a custom "Release/Remove Lock" operation
		FUnityVersionControlProvider& Provider = FUnityVersionControlModule::Get().GetProvider();
		const FString& WorkspaceRoot = Provider.GetPathToWorkspaceRoot();
		const TArray<FString> Files = UnityVersionControlUtils::LocksToFileNames(WorkspaceRoot, InSelectedLocks);
		TSharedRef<FPlasticUnlock, ESPMode::ThreadSafe> UnlockOperation = ISourceControlOperation::Create<FPlasticUnlock>();
		UnlockOperation->bRemove = bInRemove;
		UnlockOperation->Locks = MoveTemp(InSelectedLocks);
		const ECommandResult::Type Result = Provider.Execute(UnlockOperation, Files, EConcurrency::Asynchronous, FSourceControlOperationComplete::CreateRaw(this, &FUnityVersionControlMenu::OnSourceControlOperationComplete));
		if (Result == ECommandResult::Succeeded)
		{
			// Display an ongoing notification during the whole operation (packages will be reloaded at the completion of the operation)
			Notification.DisplayInProgress(UnlockOperation->GetInProgressString());
		}
		else
		{
			// Report failure with a notification (but nothing need to be reloaded since no local change is expected)
			FNotification::DisplayFailure(UnlockOperation.Get());
		}
	}
	else
	{
		FMessageLog SourceControlLog("SourceControl");
		SourceControlLog.Warning(LOCTEXT("SourceControlMenu_InProgress", "Source control operation already in progress"));
		SourceControlLog.Notify();
	}
}

bool FUnityVersionControlMenu::IsSourceControlConnected() const
{
	const ISourceControlProvider& Provider = ISourceControlModule::Get().GetProvider();
	return Provider.IsEnabled() && Provider.IsAvailable();
}

void FUnityVersionControlMenu::SyncProjectClicked()
{
	if (!Notification.IsInProgress())
	{
		// Warn the user about any unsaved assets (risk of losing work) but don't enforce saving them (reduces friction and solves some user scenario)
		PackageUtils::SaveDirtyPackages();

		// Find and Unlink all loaded packages in Content directory to allow to update them
		PackageUtils::UnlinkPackages(PackageUtils::ListAllPackages());

		// Launch a custom "SyncAll" operation
		FUnityVersionControlProvider& Provider = FUnityVersionControlModule::Get().GetProvider();
		TSharedRef<FPlasticSyncAll, ESPMode::ThreadSafe> SyncOperation = ISourceControlOperation::Create<FPlasticSyncAll>();
		const ECommandResult::Type Result = Provider.Execute(SyncOperation, TArray<FString>(), EConcurrency::Asynchronous, FSourceControlOperationComplete::CreateRaw(this, &FUnityVersionControlMenu::OnSyncAllOperationComplete));
		if (Result == ECommandResult::Succeeded)
		{
			// Display an ongoing notification during the whole operation (packages will be reloaded at the completion of the operation)
			Notification.DisplayInProgress(SyncOperation->GetInProgressString());
		}
		else
		{
			// Report failure with a notification (but nothing need to be reloaded since no local change is expected)
			FNotification::DisplayFailure(SyncOperation.Get());
		}
	}
	else
	{
		FMessageLog SourceControlLog("SourceControl");
		SourceControlLog.Warning(LOCTEXT("SourceControlMenu_InProgress", "Source control operation already in progress"));
		SourceControlLog.Notify();
	}
}

void FUnityVersionControlMenu::RevertUnchangedClicked()
{
	if (!Notification.IsInProgress())
	{
		// Launch a "RevertUnchanged" Operation
		FUnityVersionControlProvider& Provider = FUnityVersionControlModule::Get().GetProvider();
		TSharedRef<FPlasticRevertUnchanged, ESPMode::ThreadSafe> RevertUnchangedOperation = ISourceControlOperation::Create<FPlasticRevertUnchanged>();
		const ECommandResult::Type Result = Provider.Execute(RevertUnchangedOperation, TArray<FString>(), EConcurrency::Asynchronous, FSourceControlOperationComplete::CreateRaw(this, &FUnityVersionControlMenu::OnSourceControlOperationComplete));
		if (Result == ECommandResult::Succeeded)
		{
			// Display an ongoing notification during the whole operation
			Notification.DisplayInProgress(RevertUnchangedOperation->GetInProgressString());
		}
		else
		{
			// Report failure with a notification
			FNotification::DisplayFailure(RevertUnchangedOperation.Get());
		}
	}
	else
	{
		FMessageLog SourceControlLog("SourceControl");
		SourceControlLog.Warning(LOCTEXT("SourceControlMenu_InProgress", "Source control operation already in progress"));
		SourceControlLog.Notify();
	}
}

void FUnityVersionControlMenu::RevertAllClicked()
{
	if (!Notification.IsInProgress())
	{
		// Ask the user before reverting all!
		const FText AskRevertAllWarning(LOCTEXT("SourceControlMenu_AskRevertAll", "Revert all modifications into the workspace?\n"
			"This cannot be undone."));
		const EAppReturnType::Type Choice = FMessageDialog::Open(
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3
			EAppMsgCategory::Warning,
#endif
			EAppMsgType::OkCancel, AskRevertAllWarning
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3
			, LOCTEXT("SourceControlMenu_AskRevertAllTitle", "Revert All?")
#endif
		);
		if (Choice == EAppReturnType::Ok)
		{
			// Warn the user about any unsaved assets (risk of losing work) but don't enforce saving them (reduces friction and solves some user scenario)
			PackageUtils::SaveDirtyPackages();

			// Find and Unlink all packages in Content directory to allow to update them
			PackageUtils::UnlinkPackages(PackageUtils::ListAllPackages());

			// Launch a "RevertAll" Operation
			FUnityVersionControlProvider& Provider = FUnityVersionControlModule::Get().GetProvider();
			TSharedRef<FPlasticRevertAll, ESPMode::ThreadSafe> RevertAllOperation = ISourceControlOperation::Create<FPlasticRevertAll>();
			const ECommandResult::Type Result = Provider.Execute(RevertAllOperation, TArray<FString>(), EConcurrency::Asynchronous, FSourceControlOperationComplete::CreateRaw(this, &FUnityVersionControlMenu::OnRevertAllOperationComplete));
			if (Result == ECommandResult::Succeeded)
			{
				// Display an ongoing notification during the whole operation
				Notification.DisplayInProgress(RevertAllOperation->GetInProgressString());
			}
			else
			{
				// Report failure with a notification (but nothing need to be reloaded since no local change is expected)
				FNotification::DisplayFailure(RevertAllOperation.Get());
			}
		}
	}
	else
	{
		FMessageLog SourceControlLog("SourceControl");
		SourceControlLog.Warning(LOCTEXT("SourceControlMenu_InProgress", "Source control operation already in progress"));
		SourceControlLog.Notify();
	}
}

void FUnityVersionControlMenu::SwitchToPartialWorkspaceClicked()
{
	if (!Notification.IsInProgress())
	{
		// Ask the user before switching to Partial Workspace. It's not possible to switch back with local changes!
		const FText SwitchToPartialQuestion(LOCTEXT("SourceControlMenu_AskSwitchToPartialWorkspace", "Switch to Gluon partial workspace?\n"
			"Please note that in order to switch back to a regular workspace you will need to undo any local changes."));
		const EAppReturnType::Type Choice = FMessageDialog::Open(
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3
			EAppMsgCategory::Info,
#endif
			EAppMsgType::OkCancel, SwitchToPartialQuestion
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3
			, LOCTEXT("SourceControlMenu_SwitchToPartialTitle", "Switch to Gluon partial workspace?")
#endif
		);
		if (Choice == EAppReturnType::Ok)
		{
			// Launch a "SwitchToPartialWorkspace" Operation
			FUnityVersionControlProvider& Provider = FUnityVersionControlModule::Get().GetProvider();
			TSharedRef<FPlasticSwitchToPartialWorkspace, ESPMode::ThreadSafe> SwitchOperation = ISourceControlOperation::Create<FPlasticSwitchToPartialWorkspace>();
			const ECommandResult::Type Result = Provider.Execute(SwitchOperation, TArray<FString>(), EConcurrency::Asynchronous, FSourceControlOperationComplete::CreateRaw(this, &FUnityVersionControlMenu::OnSourceControlOperationComplete));
			if (Result == ECommandResult::Succeeded)
			{
				// Display an ongoing notification during the whole operation
				Notification.DisplayInProgress(SwitchOperation->GetInProgressString());
			}
			else
			{
				// Report failure with a notification
				FNotification::DisplayFailure(SwitchOperation.Get());
			}
		}
	}
	else
	{
		FMessageLog SourceControlLog("SourceControl");
		SourceControlLog.Warning(LOCTEXT("SourceControlMenu_InProgress", "Source control operation already in progress"));
		SourceControlLog.Notify();
	}
}

bool FUnityVersionControlMenu::CanSwitchToPartialWorkspace() const
{
	return !FUnityVersionControlModule::Get().GetProvider().IsPartialWorkspace();
}

void FUnityVersionControlMenu::ShowSourceControlEditorPreferences() const
{
	if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
	{
		SettingsModule->ShowViewer("Editor", "General", "LoadingSaving");
	}
}

void FUnityVersionControlMenu::ShowSourceControlProjectSettings() const
{
	if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
	{
		SettingsModule->ShowViewer("Project", "Editor", "SourceControlPreferences");
	}
}

void FUnityVersionControlMenu::ShowSourceControlPlasticScmProjectSettings() const
{
	if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
	{
		SettingsModule->ShowViewer("Project", "Editor", "UnityVersionControlProjectSettings");
	}
}

void FUnityVersionControlMenu::VisitDocsURLClicked() const
{
	// Grab the URL from the uplugin file
	const TSharedPtr<IPlugin> Plugin = FUnityVersionControlModule::GetPlugin();
	if (Plugin.IsValid())
	{
		FPlatformProcess::LaunchURL(*Plugin->GetDescriptor().DocsURL, NULL, NULL);
	}
}

void FUnityVersionControlMenu::VisitSupportURLClicked() const
{
	// Grab the URL from the uplugin file
	const TSharedPtr<IPlugin> Plugin = FUnityVersionControlModule::GetPlugin();
	if (Plugin.IsValid())
	{
		FPlatformProcess::LaunchURL(*Plugin->GetDescriptor().SupportURL, NULL, NULL);
	}
}

void FUnityVersionControlMenu::VisitLockRulesURLClicked(const FString InOrganizationName) const
{
	UnityVersionControlUtils::OpenLockRulesInCloudDashboard(InOrganizationName);
}

void FUnityVersionControlMenu::OpenDesktopApplication() const
{
	UnityVersionControlUtils::OpenDesktopApplication();
}

void FUnityVersionControlMenu::OpenBranchesWindow() const
{
	FUnityVersionControlModule::Get().GetBranchesWindow().OpenTab();
}

void FUnityVersionControlMenu::OpenChangesetsWindow() const
{
	FUnityVersionControlModule::Get().GetChangesetsWindow().OpenTab();
}

void FUnityVersionControlMenu::OpenLocksWindow() const
{
	FUnityVersionControlModule::Get().GetLocksWindow().OpenTab();
}

void FUnityVersionControlMenu::OnSyncAllOperationComplete(const FSourceControlOperationRef& InOperation, ECommandResult::Type InResult)
{
	OnSourceControlOperationComplete(InOperation, InResult);

	// Reload packages that where updated by the Sync operation (and the current map if needed)
	TSharedRef<FPlasticSyncAll, ESPMode::ThreadSafe> Operation = StaticCastSharedRef<FPlasticSyncAll>(InOperation);
	PackageUtils::ReloadPackages(Operation->UpdatedFiles);
}

void FUnityVersionControlMenu::OnRevertAllOperationComplete(const FSourceControlOperationRef & InOperation, ECommandResult::Type InResult)
{
	OnSourceControlOperationComplete(InOperation, InResult);

	// Reload packages that where updated by the Revert operation (and the current map if needed)
	TSharedRef<FPlasticRevertAll, ESPMode::ThreadSafe> Operation = StaticCastSharedRef<FPlasticRevertAll>(InOperation);
	PackageUtils::ReloadPackages(Operation->UpdatedFiles);
}

void FUnityVersionControlMenu::OnSourceControlOperationComplete(const FSourceControlOperationRef& InOperation, ECommandResult::Type InResult)
{
	Notification.RemoveInProgress();

	FNotification::DisplayResult(InOperation, InResult);
}

#if ENGINE_MAJOR_VERSION == 4
void FUnityVersionControlMenu::AddMenuExtension(FMenuBuilder& Menu)
#elif ENGINE_MAJOR_VERSION == 5
void FUnityVersionControlMenu::AddMenuExtension(FToolMenuSection& Menu)
#endif
{
	FUnityVersionControlProvider& Provider = FUnityVersionControlModule::Get().GetProvider();

	Menu.AddMenuEntry(
#if ENGINE_MAJOR_VERSION == 5
		"PlasticSync",
#endif
		LOCTEXT("PlasticSync",			"Sync/Update Workspace"),
		LOCTEXT("PlasticSyncTooltip",	"Update the workspace to the latest changeset of the branch, and reload all affected assets."),
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "SourceControl.Actions.Sync"),
#else
		FSlateIcon(FEditorStyle::GetStyleSetName(), "SourceControl.Actions.Sync"),
#endif
		FUIAction(
			FExecuteAction::CreateRaw(this, &FUnityVersionControlMenu::SyncProjectClicked),
			FCanExecuteAction()
		)
	);

	Menu.AddMenuEntry(
#if ENGINE_MAJOR_VERSION == 5
		"PlasticRevertUnchanged",
#endif
		LOCTEXT("PlasticRevertUnchanged",			"Revert Unchanged"),
		LOCTEXT("PlasticRevertUnchangedTooltip",	"Revert checked-out but unchanged files in the workspace."),
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "SourceControl.Actions.Revert"),
#else
		FSlateIcon(FEditorStyle::GetStyleSetName(), "SourceControl.Actions.Revert"),
#endif
		FUIAction(
			FExecuteAction::CreateRaw(this, &FUnityVersionControlMenu::RevertUnchangedClicked),
			FCanExecuteAction()
		)
	);

	Menu.AddMenuEntry(
#if ENGINE_MAJOR_VERSION == 5
		"PlasticRevertAll",
#endif
		LOCTEXT("PlasticRevertAll",			"Revert All"),
		LOCTEXT("PlasticRevertAllTooltip",	"Revert all files in the workspace to their controlled/unchanged state."),
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "SourceControl.Actions.Revert"),
#else
		FSlateIcon(FEditorStyle::GetStyleSetName(), "SourceControl.Actions.Revert"),
#endif
		FUIAction(
			FExecuteAction::CreateRaw(this, &FUnityVersionControlMenu::RevertAllClicked),
			FCanExecuteAction()
		)
	);

	Menu.AddMenuEntry(
#if ENGINE_MAJOR_VERSION == 5
		"SwitchToPartialWorkspace",
#endif
		LOCTEXT("SwitchToPartialWorkspace",			"Switch to Gluon Partial Workspace"),
		LOCTEXT("SwitchToPartialWorkspaceTooltip",	"Update the workspace to a Gluon partial mode for a simplified workflow.\n"
			"Allows to update and check in files individually as opposed to the whole workspace.\nIt doesn't work with branches or shelves."),
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "GenericCommands.Cut"),
#else
		FSlateIcon(FEditorStyle::GetStyleSetName(), "GenericCommands.Cut"),
#endif
		FUIAction(
			FExecuteAction::CreateRaw(this, &FUnityVersionControlMenu::SwitchToPartialWorkspaceClicked),
			FCanExecuteAction::CreateRaw(this, &FUnityVersionControlMenu::CanSwitchToPartialWorkspace)
		)
	);

	Menu.AddMenuEntry(
#if ENGINE_MAJOR_VERSION == 5
		"SourceControlEditorPreferences",
#endif
		LOCTEXT("SourceControlEditorPreferences", "Editor Preferences - Source Control"),
		LOCTEXT("SourceControlEditorPreferencesTooltip", "Open the Load & Save section with Source Control in the Editor Preferences."),
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "EditorPreferences.TabIcon"),
#else
		FSlateIcon(FEditorStyle::GetStyleSetName(), "EditorPreferences.TabIcon"),
#endif
		FUIAction(
			FExecuteAction::CreateRaw(this, &FUnityVersionControlMenu::ShowSourceControlEditorPreferences),
			FCanExecuteAction()
		)
	);

#if ENGINE_MAJOR_VERSION == 5 // Disable the "Source Control Project Settings" for UE4 since this section is new to UE5
	Menu.AddMenuEntry(
		"SourceControlProjectSettings",
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 2
		LOCTEXT("SourceControlProjectSettings",			"Project Settings - Revision Control"),
		LOCTEXT("SourceControlProjectSettingsTooltip",	"Open the Revision Control section in the Project Settings."),
#else
		LOCTEXT("SourceControlProjectSettings",			"Project Settings - Source Control"),
		LOCTEXT("SourceControlProjectSettingsTooltip",	"Open the Source Control section in the Project Settings."),
#endif
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "ProjectSettings.TabIcon"),
#else
		FSlateIcon(FEditorStyle::GetStyleSetName(), "ProjectSettings.TabIcon"),
#endif
		FUIAction(
			FExecuteAction::CreateRaw(this, &FUnityVersionControlMenu::ShowSourceControlProjectSettings),
			FCanExecuteAction()
		)
	);
#endif

	Menu.AddMenuEntry(
#if ENGINE_MAJOR_VERSION == 5
		"PlasticProjectSettings",
#endif
		LOCTEXT("PlasticProjectSettings",			"Project Settings - Source Control - Unity Version Control"),
		LOCTEXT("PlasticProjectSettingsTooltip",	"Open the Unity Version Control (formerly Plastic SCM) section in the Project Settings."),
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "ProjectSettings.TabIcon"),
#else
		FSlateIcon(FEditorStyle::GetStyleSetName(), "ProjectSettings.TabIcon"),
#endif
		FUIAction(
			FExecuteAction::CreateRaw(this, &FUnityVersionControlMenu::ShowSourceControlPlasticScmProjectSettings),
			FCanExecuteAction()
		)
	);

	Menu.AddMenuEntry(
#if ENGINE_MAJOR_VERSION == 5
		"PlasticDocsURL",
#endif
		LOCTEXT("PlasticDocsURL",			"Plugin's Documentation"),
		LOCTEXT("PlasticDocsURLTooltip",	"Visit documentation of the plugin on Github."),
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Documentation"),
#elif ENGINE_MAJOR_VERSION == 5
		FSlateIcon(FEditorStyle::GetStyleSetName(), "Icons.Documentation"),
#elif ENGINE_MAJOR_VERSION == 4
		FSlateIcon(FEditorStyle::GetStyleSetName(), "LevelEditor.BrowseDocumentation"),
#endif
		FUIAction(
			FExecuteAction::CreateRaw(this, &FUnityVersionControlMenu::VisitDocsURLClicked),
			FCanExecuteAction()
		)
	);

	Menu.AddMenuEntry(
#if ENGINE_MAJOR_VERSION == 5
		"PlasticSupportURL",
#endif
		LOCTEXT("PlasticSupportURL",		"Unity Version Control Support"),
		LOCTEXT("PlasticSupportURLTooltip",	"Submit a support request for Unity Version Control (formerly Plastic SCM)."),
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Support"),
#elif ENGINE_MAJOR_VERSION == 5
		FSlateIcon(FEditorStyle::GetStyleSetName(), "Icons.Support"),
#elif ENGINE_MAJOR_VERSION == 4
		FSlateIcon(FEditorStyle::GetStyleSetName(), "LevelEditor.BrowseDocumentation"),
#endif
		FUIAction(
			FExecuteAction::CreateRaw(this, &FUnityVersionControlMenu::VisitSupportURLClicked),
			FCanExecuteAction()
		)
	);

	FString OrganizationName = FUnityVersionControlModule::Get().GetProvider().GetCloudOrganization();
	if (!OrganizationName.IsEmpty())
	{
		Menu.AddMenuEntry(
#if ENGINE_MAJOR_VERSION == 5
			"PlasticLockRulesURL",
#endif
			LOCTEXT("PlasticLockRulesURL", "Configure Lock Rules"),
			LOCTEXT("PlasticLockRulesURLTooltip", "Navigate to lock rules configuration page in the Unity Dashboard."),
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "PropertyWindow.Locked"),
#else
			FSlateIcon(FEditorStyle::GetStyleSetName(), "PropertyWindow.Locked"),
#endif
			FUIAction(
				FExecuteAction::CreateRaw(this, &FUnityVersionControlMenu::VisitLockRulesURLClicked, MoveTemp(OrganizationName)),
				FCanExecuteAction()
			)
		);
	}

	Menu.AddMenuEntry(
#if ENGINE_MAJOR_VERSION == 5
		"PlasticDesktopApp",
		TAttribute<FText>::CreateLambda([&Provider]()
			{
				return Provider.IsPartialWorkspace() ? LOCTEXT("PlasticGluon", "Open in Gluon") : LOCTEXT("PlasticDesktopApp", "Open in Desktop App");
			}),
		TAttribute<FText>::CreateLambda([&Provider]()
			{
				return Provider.IsPartialWorkspace() ? LOCTEXT("PlasticDesktopAppTooltip", "Open the workspace in Unity Version Control Gluon Application.") : LOCTEXT("PlasticGluonTooltip", "Open the workspace in Unity Version Control Desktop Application.");
			}),
		TAttribute<FSlateIcon>::CreateLambda([&Provider]()
			{
				return FSlateIcon(FUnityVersionControlStyle::Get().GetStyleSetName(), Provider.IsPartialWorkspace() ? "UnityVersionControl.GluonIcon.Small" : "UnityVersionControl.PluginIcon.Small");
			}),
#else
		Provider.IsPartialWorkspace() ? LOCTEXT("PlasticGluon", "Open in Gluon") : LOCTEXT("PlasticDesktopApp", "Open in Desktop App"),
		Provider.IsPartialWorkspace() ? LOCTEXT("PlasticDesktopAppTooltip", "Open the workspace in Unity Version Control Gluon Application.") : LOCTEXT("PlasticGluonTooltip", "Open the workspace in Unity Version Control Desktop Application."),
		FSlateIcon(FUnityVersionControlStyle::Get().GetStyleSetName(), Provider.IsPartialWorkspace() ? "UnityVersionControl.GluonIcon.Small" : "UnityVersionControl.PluginIcon.Small"),
#endif
		FUIAction(FExecuteAction::CreateRaw(this, &FUnityVersionControlMenu::OpenDesktopApplication))
	);

	AddViewBranches(Menu);
	AddViewChangesets(Menu);
	AddViewLocks(Menu);
}

#if ENGINE_MAJOR_VERSION == 4
void FUnityVersionControlMenu::AddViewBranches(FMenuBuilder& Menu)
#elif ENGINE_MAJOR_VERSION == 5
void FUnityVersionControlMenu::AddViewBranches(FToolMenuSection& Menu)
#endif
{
	Menu.AddMenuEntry(
#if ENGINE_MAJOR_VERSION == 5
		TEXT("PlasticBranchesWindow"),
#endif
		LOCTEXT("PlasticBranchesWindow", "View Branches"),
		LOCTEXT("PlasticBranchesWindowTooltip", "Open the Branches window."),
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "SourceControl.Branch"),
#else
		FSlateIcon(FEditorStyle::GetStyleSetName(), "SourceControl.Branch"),
#endif
		FUIAction(
			FExecuteAction::CreateRaw(this, &FUnityVersionControlMenu::OpenBranchesWindow),
			FCanExecuteAction()
		)
	);
}

#if ENGINE_MAJOR_VERSION == 4
void FUnityVersionControlMenu::AddViewChangesets(FMenuBuilder& Menu)
#elif ENGINE_MAJOR_VERSION == 5
void FUnityVersionControlMenu::AddViewChangesets(FToolMenuSection& Menu)
#endif
{
	Menu.AddMenuEntry(
#if ENGINE_MAJOR_VERSION == 5
		TEXT("PlasticChangesetsWindow"),
#endif
		LOCTEXT("PlasticChangesetsWindow", "View Changesets"),
		LOCTEXT("PlasticChangesetsWindowTooltip", "Open the Changesets window."),
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "SourceControl.Actions.History"),
#else
		FSlateIcon(FEditorStyle::GetStyleSetName(), "SourceControl.Actions.History"),
#endif
		FUIAction(
			FExecuteAction::CreateRaw(this, &FUnityVersionControlMenu::OpenChangesetsWindow),
			FCanExecuteAction()
		)
	);
}

#if ENGINE_MAJOR_VERSION == 4
void FUnityVersionControlMenu::AddViewLocks(FMenuBuilder& Menu)
#elif ENGINE_MAJOR_VERSION == 5
void FUnityVersionControlMenu::AddViewLocks(FToolMenuSection& Menu)
#endif
{
	const bool bVersionSupportsSmartLocks = FUnityVersionControlModule::Get().GetProvider().GetPlasticScmVersion() >= UnityVersionControlVersions::SmartLocks;

	Menu.AddMenuEntry(
#if ENGINE_MAJOR_VERSION == 5
		TEXT("PlasticLocksWindow"),
#endif
		LOCTEXT("PlasticLocksWindow", "View Locks"),
		LOCTEXT("PlasticLocksWindowTooltip", "Open the Locks window."),
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "PropertyWindow.Locked"),
#else
		FSlateIcon(FEditorStyle::GetStyleSetName(), "PropertyWindow.Locked"),
#endif
		FUIAction(
			FExecuteAction::CreateRaw(this, &FUnityVersionControlMenu::OpenLocksWindow),
			FCanExecuteAction::CreateLambda([bVersionSupportsSmartLocks]() { return bVersionSupportsSmartLocks; })
		)
	);
}

#if ENGINE_MAJOR_VERSION == 4
TSharedRef<FExtender> FUnityVersionControlMenu::OnExtendLevelEditorViewMenu(const TSharedRef<FUICommandList> CommandList)
{
	TSharedRef<FExtender> Extender(new FExtender());

	Extender->AddMenuExtension(
		"SourceControlActions",
		EExtensionHook::After,
		nullptr,
		FMenuExtensionDelegate::CreateRaw(this, &FUnityVersionControlMenu::AddMenuExtension));

	return Extender;
}
#endif

#undef LOCTEXT_NAMESPACE
