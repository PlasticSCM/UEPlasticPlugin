// Copyright (c) 2023 Unity Technologies

#include "PlasticSourceControlMenu.h"

#if SOURCE_CONTROL_WITH_SLATE

#include "PlasticSourceControlModule.h"
#include "PlasticSourceControlProvider.h"
#include "PlasticSourceControlOperations.h"

#include "ISourceControlModule.h"
#include "ISourceControlOperation.h"
#include "SourceControlOperations.h"

#include "ContentBrowserMenuContexts.h"
#include "Interfaces/IPluginManager.h"
#include "Modules/ModuleManager.h"
#include "LevelEditor.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Misc/MessageDialog.h"
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
#include "Styling/AppStyle.h"
#else
#include "EditorStyleSet.h"
#endif

#include "PackageUtils.h"
#include "FileHelpers.h"
#include "ISettingsModule.h"

#include "Logging/MessageLog.h"

#include "ToolMenus.h"
#include "ToolMenuContext.h"
#include "ToolMenuMisc.h"

#define LOCTEXT_NAMESPACE "PlasticSourceControl"

static const FName UnityVersionControlMainMenuOwnerName = TEXT("UnityVersionControlMenu");
static const FName UnityVersionControlAssetContextLocksMenuOwnerName = TEXT("UnityVersionControlContextLocksMenu");

void FPlasticSourceControlMenu::Register()
{
	if (bHasRegistered)
	{
		return;
	}

	// Register the menu extensions with the level editor
	ExtendRevisionControlMenu();
	ExtendAssetContextMenu();
}

void FPlasticSourceControlMenu::Unregister()
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
		bHasRegistered = false;
	}
#endif
}


void FPlasticSourceControlMenu::ExtendRevisionControlMenu()
{
#if ENGINE_MAJOR_VERSION == 4
	if (FLevelEditorModule* LevelEditorModule = FModuleManager::GetModulePtr<FLevelEditorModule>("LevelEditor"))
	{
		FLevelEditorModule::FLevelEditorMenuExtender ViewMenuExtender = FLevelEditorModule::FLevelEditorMenuExtender::CreateRaw(this, &FPlasticSourceControlMenu::OnExtendLevelEditorViewMenu);
		auto& MenuExtenders = LevelEditorModule->GetAllLevelEditorToolbarSourceControlMenuExtenders();
		MenuExtenders.Add(ViewMenuExtender);
		ViewMenuExtenderHandle = MenuExtenders.Last().GetHandle();

		bHasRegistered = true;
	}
#elif ENGINE_MAJOR_VERSION == 5
	const FToolMenuOwnerScoped SourceControlMenuOwner(UnityVersionControlMainMenuOwnerName);
	if (UToolMenus* ToolMenus = UToolMenus::Get())
	{
		if (UToolMenu* SourceControlMenu = ToolMenus->ExtendMenu("StatusBar.ToolBar.SourceControl"))
		{
			// Merge the main actions into the existing source control actions, at the top of the menu
			// TODO POC REVIEW is it really a good idea to change anything of it? It turns out that I don't think so
//			FToolMenuSection& ActionsSection = SourceControlMenu->FindOrAddSection("SourceControlActions");

			// Create a dedicated section for Unity Version Control submenus
			FToolMenuSection& UnityVersionControlSection = SourceControlMenu->AddSection("UnityVersionControlSection", LOCTEXT("UnityVersionControlMenuHeading", "Unity Version Control"), FToolMenuInsert(NAME_None, EToolMenuInsertType::First));
			UnityVersionControlSection.AddSubMenu(
				TEXT("PlasticSettingsSubMenu"),
				LOCTEXT("PlasticSettingsSubMenu", "Settings"),
				FText::GetEmpty(),
				FNewMenuDelegate::CreateRaw(this, &FPlasticSourceControlMenu::GeneratePlasticSettingsMenu),
				false,
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
				FSlateIcon(FAppStyle::GetAppStyleSetName(), "ProjectSettings.TabIcon")
#else
				FSlateIcon(FEditorStyle::GetStyleSetName(), "ProjectSettings.TabIcon")
#endif
			);

			UnityVersionControlSection.AddSubMenu(
				TEXT("PlasticWebLinksSubMenu"),
				LOCTEXT("PlasticWebLinksSubMenu", "Web Links"),
				FText::GetEmpty(),
				FNewMenuDelegate::CreateRaw(this, &FPlasticSourceControlMenu::GeneratePlasticWebLinksMenu),
				false,
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
				FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Documentation")
#elif ENGINE_MAJOR_VERSION == 5
				FSlateIcon(FEditorStyle::GetStyleSetName(), "Icons.Documentation")
#elif ENGINE_MAJOR_VERSION == 4
				FSlateIcon(FEditorStyle::GetStyleSetName(), "LevelEditor.BrowseDocumentation")
#endif
			);
			
			UnityVersionControlSection.AddSubMenu(
				TEXT("PlasticAdvancedSubMenu"),
				LOCTEXT("PlasticAdvancedSubMenu", "Advanced"),
				FText::GetEmpty(),
				FNewMenuDelegate::CreateRaw(this, &FPlasticSourceControlMenu::GeneratePlasticAdvancedMenu),
				false,
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
				FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Documentation")
#elif ENGINE_MAJOR_VERSION == 5
				FSlateIcon(FEditorStyle::GetStyleSetName(), "Icons.Documentation")
#elif ENGINE_MAJOR_VERSION == 4
				FSlateIcon(FEditorStyle::GetStyleSetName(), "LevelEditor.BrowseDocumentation")
#endif
			);

			UnityVersionControlSection.AddSeparator("PlasticSeparator");

			// TODO POC REVIEW at the end of the Unity Version Control section so it's close to the original Source Control Actions
			AddMenuActions(UnityVersionControlSection);

			// TODO: try to also extend "SourceControlSubMenu", to add another section below "AssetSourceControlActions"
			// Note: inspired by FConcertWorkspaceUI::ExtendAssetContextMenuOptions()
			ExtendToolbarWithSourceControlMenu();

			bHasRegistered = true;
		}
	}
#endif
}


void FPlasticSourceControlMenu::ExtendToolbarWithSourceControlMenu()
{
	// TODO	const FToolMenuOwnerScoped SourceControlMenuOwner(PlasticBranchToolbarOwnerName);

	/// Inspired by SLevelEditor::RegisterStatusBarTools()
	/// => see also SPlasticSourceControlStatusBar

	UToolMenu* ToolbarMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.StatusBar.ToolBar");
	FToolMenuSection& Section = ToolbarMenu->AddSection("Unity Version Control", FText::GetEmpty(), FToolMenuInsert("SourceControl", EToolMenuInsertType::Before));

	Section.AddEntry(
		FToolMenuEntry::InitWidget("UnityVersionControlStatusBar", CreateStatusBarWidget(), FText::GetEmpty(), true, false)
	);
}


// TODO move that to a separate file
// See SPlasticSourceControlStatusBar

TSharedRef<SWidget> FPlasticSourceControlMenu::CreateStatusBarWidget()
{
	return SNew(SPlasticSourceControlStatusBar);
}


void FPlasticSourceControlMenu::ExtendAssetContextMenu()
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
			if (!Context || !Context->bCanBeModified || Context->SelectedObjects.Num() == 0 || !ensure(FPlasticSourceControlModule::IsLoaded()))
			{
				return;
			}
			TArray<FAssetData> AssetObjectPaths;
			AssetObjectPaths.Reserve(Context->SelectedObjects.Num());
			for (const auto SelectedObject : SelectedObjects)
			{
			// TODO: remove this transformation since we want to use the AssetData directly!
				AssetObjectPaths.Add(FAssetData(SelectedObject));
			}
#else
			if (!Context || !Context->bCanBeModified || Context->SelectedAssets.Num() == 0 || !ensure(FPlasticSourceControlModule::IsLoaded()))
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
				FNewMenuDelegate::CreateRaw(this, &FPlasticSourceControlMenu::GeneratePlasticAssetContextMenu, MoveTemp(AssetObjectPaths)),
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

void FPlasticSourceControlMenu::GeneratePlasticAssetContextMenu(FMenuBuilder& MenuBuilder, TArray<FAssetData> InAssetObjectPaths)
{
	MenuBuilder.BeginSection("AssetPlasticActions", LOCTEXT("UnityVersionControlAssetContextLocksMenuHeading", "Unity Version Control Locks"));

	{
		MenuBuilder.AddMenuEntry(
			LOCTEXT("PlasticReleaseLock", "Release Lock"),
			LOCTEXT("PlasticReleaseLockTooltip", "Release Lock(s) on the selected assets. Requires administrator privileges on the server."),
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "PropertyWindow.Unlocked"),
#else
			FSlateIcon(FEditorStyle::GetStyleSetName(), "PropertyWindow.Unlocked"),
#endif
			FUIAction(
				FExecuteAction::CreateRaw(this, &FPlasticSourceControlMenu::ExecuteReleaseLocks, InAssetObjectPaths),
				FCanExecuteAction::CreateRaw(this, &FPlasticSourceControlMenu::CanReleaseLocks, InAssetObjectPaths)
			)
		);
	}

	{
		MenuBuilder.AddMenuEntry(
			LOCTEXT("PlasticRemoveLock", "Remove Lock"),
			LOCTEXT("PlasticRemoveLockTooltip", "Remove/Delete Lock(s) on the selected assets. Requires administrator privileges on the server."),
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "PropertyWindow.Unlocked"),
#else
			FSlateIcon(FEditorStyle::GetStyleSetName(), "PropertyWindow.Unlocked"),
#endif
			FUIAction(
				FExecuteAction::CreateRaw(this, &FPlasticSourceControlMenu::ExecuteRemoveLocks, InAssetObjectPaths),
				FCanExecuteAction::CreateRaw(this, &FPlasticSourceControlMenu::CanRemoveLocks, InAssetObjectPaths)
			)
		);
	}

	FString OrganizationName = FPlasticSourceControlModule::Get().GetProvider().GetCloudOrganization();
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
				FExecuteAction::CreateRaw(this, &FPlasticSourceControlMenu::VisitLockRulesURLClicked, MoveTemp(OrganizationName)),
				FCanExecuteAction()
			)
		);
	}

	MenuBuilder.EndSection();
}

bool FPlasticSourceControlMenu::CanReleaseLocks(TArray<FAssetData> InAssetObjectPaths) const
{
	const TArray<FString> Files = PackageUtils::AssetDateToFileNames(InAssetObjectPaths);

	for (const FString& File : Files)
	{
		const FString AbsoluteFilename = FPaths::ConvertRelativePathToFull(File);
		const auto State = FPlasticSourceControlModule::Get().GetProvider().GetStateInternal(AbsoluteFilename);
		// If exclusively Checked Out (Locked) the lock can be released coming back to it's potential underlying "Retained" status if changes where already checked in the branch
		if (!State->LockedBy.IsEmpty() && State->LockedId != ISourceControlState::INVALID_REVISION)
		{
			return true;
		}
	}

	return false;
}

bool FPlasticSourceControlMenu::CanRemoveLocks(TArray<FAssetData> InAssetObjectPaths) const
{
	const TArray<FString> Files = PackageUtils::AssetDateToFileNames(InAssetObjectPaths);

	for (const FString& File : Files)
	{
		const FString AbsoluteFilename = FPaths::ConvertRelativePathToFull(File);
		const auto State = FPlasticSourceControlModule::Get().GetProvider().GetStateInternal(AbsoluteFilename);
		// If Locked or Retained, the lock can be removed, that is completely deleted in order to simply ignore the changes from the branch
		if (State->LockedId != ISourceControlState::INVALID_REVISION)
		{
			return true;
		}
	}

	return false;
}

void FPlasticSourceControlMenu::ExecuteReleaseLocks(TArray<FAssetData> InAssetObjectPaths)
{
	ExecuteUnlock(InAssetObjectPaths, false);
}

void FPlasticSourceControlMenu::ExecuteRemoveLocks(TArray<FAssetData> InAssetObjectPaths)
{
	ExecuteUnlock(InAssetObjectPaths, true);
}

void FPlasticSourceControlMenu::ExecuteUnlock(const TArray<FAssetData>& InAssetObjectPaths, const bool bInRemove)
{
	if (!OperationInProgressNotification.IsValid())
	{
		const TArray<FString> Files = PackageUtils::AssetDateToFileNames(InAssetObjectPaths);

		// Launch a custom "Release/Remove Lock" operation
		FPlasticSourceControlProvider& Provider = FPlasticSourceControlModule::Get().GetProvider();
		TSharedRef<FPlasticUnlock, ESPMode::ThreadSafe> UnlockOperation = ISourceControlOperation::Create<FPlasticUnlock>();
		const ECommandResult::Type Result = Provider.Execute(UnlockOperation, Files, EConcurrency::Asynchronous, FSourceControlOperationComplete::CreateRaw(this, &FPlasticSourceControlMenu::OnSourceControlOperationComplete));
		UnlockOperation->bRemove = bInRemove;
		if (Result == ECommandResult::Succeeded)
		{
			// Display an ongoing notification during the whole operation (packages will be reloaded at the completion of the operation)
			DisplayInProgressNotification(UnlockOperation->GetInProgressString());
		}
		else
		{
			// Report failure with a notification (but nothing need to be reloaded since no local change is expected)
			DisplayFailureNotification(UnlockOperation->GetName());
		}
	}
	else
	{
		FMessageLog SourceControlLog("SourceControl");
		SourceControlLog.Warning(LOCTEXT("SourceControlMenu_InProgress", "Source control operation already in progress"));
		SourceControlLog.Notify();
	}
}

bool FPlasticSourceControlMenu::IsSourceControlConnected() const
{
	const ISourceControlProvider& Provider = ISourceControlModule::Get().GetProvider();
	return Provider.IsEnabled() && Provider.IsAvailable();
}

/// Prompt to save or discard all packages
bool FPlasticSourceControlMenu::SaveDirtyPackages()
{
	const bool bPromptUserToSave = true;
	const bool bSaveMapPackages = true;
	const bool bSaveContentPackages = true;
	const bool bFastSave = false;
	const bool bNotifyNoPackagesSaved = false;
	const bool bCanBeDeclined = true; // If the user clicks "don't save" this will continue and lose their changes
	bool bHadPackagesToSave = false;

	bool bSaved = FEditorFileUtils::SaveDirtyPackages(bPromptUserToSave, bSaveMapPackages, bSaveContentPackages, bFastSave, bNotifyNoPackagesSaved, bCanBeDeclined, &bHadPackagesToSave);

	// bSaved can be true if the user selects to not save an asset by un-checking it and clicking "save"
	if (bSaved)
	{
		TArray<UPackage*> DirtyPackages;
		FEditorFileUtils::GetDirtyWorldPackages(DirtyPackages);
		FEditorFileUtils::GetDirtyContentPackages(DirtyPackages);
		bSaved = DirtyPackages.Num() == 0;
	}

	return bSaved;
}

/// Find all packages in Content directory
TArray<FString> FPlasticSourceControlMenu::ListAllPackages()
{
	TArray<FString> PackageFilePaths;
	FPackageName::FindPackagesInDirectory(PackageFilePaths, FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir()));
	return PackageFilePaths;
}

void FPlasticSourceControlMenu::SyncProjectClicked()
{
	if (!OperationInProgressNotification.IsValid())
	{
		const bool bSaved = SaveDirtyPackages();
		if (bSaved)
		{
			// Find and Unlink all loaded packages in Content directory to allow to update them
			PackageUtils::UnlinkPackages(ListAllPackages());

			// Launch a custom "SyncAll" operation
			FPlasticSourceControlProvider& Provider = FPlasticSourceControlModule::Get().GetProvider();
			TSharedRef<FPlasticSyncAll, ESPMode::ThreadSafe> SyncOperation = ISourceControlOperation::Create<FPlasticSyncAll>();
			const ECommandResult::Type Result = Provider.Execute(SyncOperation, TArray<FString>(), EConcurrency::Asynchronous, FSourceControlOperationComplete::CreateRaw(this, &FPlasticSourceControlMenu::OnSyncAllOperationComplete));
			if (Result == ECommandResult::Succeeded)
			{
				// Display an ongoing notification during the whole operation (packages will be reloaded at the completion of the operation)
				DisplayInProgressNotification(SyncOperation->GetInProgressString());
			}
			else
			{
				// Report failure with a notification (but nothing need to be reloaded since no local change is expected)
				DisplayFailureNotification(SyncOperation->GetName());
			}
		}
		else
		{
			FMessageLog SourceControlLog("SourceControl");
			SourceControlLog.Warning(LOCTEXT("SourceControlMenu_Sync_Unsaved", "Save All Assets before attempting to Sync!"));
			SourceControlLog.Notify();
		}
	}
	else
	{
		FMessageLog SourceControlLog("SourceControl");
		SourceControlLog.Warning(LOCTEXT("SourceControlMenu_InProgress", "Source control operation already in progress"));
		SourceControlLog.Notify();
	}
}

void FPlasticSourceControlMenu::RevertUnchangedClicked()
{
	if (!OperationInProgressNotification.IsValid())
	{
		// Launch a "RevertUnchanged" Operation
		FPlasticSourceControlProvider& Provider = FPlasticSourceControlModule::Get().GetProvider();
		TSharedRef<FPlasticRevertUnchanged, ESPMode::ThreadSafe> RevertUnchangedOperation = ISourceControlOperation::Create<FPlasticRevertUnchanged>();
		const ECommandResult::Type Result = Provider.Execute(RevertUnchangedOperation, TArray<FString>(), EConcurrency::Asynchronous, FSourceControlOperationComplete::CreateRaw(this, &FPlasticSourceControlMenu::OnSourceControlOperationComplete));
		if (Result == ECommandResult::Succeeded)
		{
			// Display an ongoing notification during the whole operation
			DisplayInProgressNotification(RevertUnchangedOperation->GetInProgressString());
		}
		else
		{
			// Report failure with a notification
			DisplayFailureNotification(RevertUnchangedOperation->GetName());
		}
	}
	else
	{
		FMessageLog SourceControlLog("SourceControl");
		SourceControlLog.Warning(LOCTEXT("SourceControlMenu_InProgress", "Source control operation already in progress"));
		SourceControlLog.Notify();
	}
}

void FPlasticSourceControlMenu::RevertAllClicked()
{
	if (!OperationInProgressNotification.IsValid())
	{
		// Ask the user before reverting all!
		const FText DialogText(LOCTEXT("SourceControlMenu_AskRevertAll", "Revert all modifications into the workspace?"));
		const EAppReturnType::Type Choice = FMessageDialog::Open(EAppMsgType::OkCancel, DialogText);
		if (Choice == EAppReturnType::Ok)
		{
			const bool bSaved = SaveDirtyPackages();
			if (bSaved)
			{
				// Find and Unlink all packages in Content directory to allow to update them
				PackageUtils::UnlinkPackages(ListAllPackages());

				// Launch a "RevertAll" Operation
				FPlasticSourceControlProvider& Provider = FPlasticSourceControlModule::Get().GetProvider();
				TSharedRef<FPlasticRevertAll, ESPMode::ThreadSafe> RevertAllOperation = ISourceControlOperation::Create<FPlasticRevertAll>();
				const ECommandResult::Type Result = Provider.Execute(RevertAllOperation, TArray<FString>(), EConcurrency::Asynchronous, FSourceControlOperationComplete::CreateRaw(this, &FPlasticSourceControlMenu::OnRevertAllOperationComplete));
				if (Result == ECommandResult::Succeeded)
				{
					// Display an ongoing notification during the whole operation
					DisplayInProgressNotification(RevertAllOperation->GetInProgressString());
				}
				else
				{
					// Report failure with a notification (but nothing need to be reloaded since no local change is expected)
					DisplayFailureNotification(RevertAllOperation->GetName());
				}
			}
			else
			{
				FMessageLog SourceControlLog("SourceControl");
				SourceControlLog.Warning(LOCTEXT("SourceControlMenu_Sync_Unsaved", "Save All Assets before attempting to Sync!"));
				SourceControlLog.Notify();
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

void FPlasticSourceControlMenu::RefreshClicked()
{
	if (!OperationInProgressNotification.IsValid())
	{
		// Launch an "UpdateStatus" Operation
		FPlasticSourceControlProvider& Provider = FPlasticSourceControlModule::Get().GetProvider();
		TSharedRef<FUpdateStatus, ESPMode::ThreadSafe> RefreshOperation = ISourceControlOperation::Create<FUpdateStatus>();
		// This is the flag used by the Content Browser's "Checkout" filter to trigger a full status update
		RefreshOperation->SetGetOpenedOnly(true);
		const ECommandResult::Type Result = Provider.Execute(RefreshOperation, TArray<FString>(), EConcurrency::Asynchronous, FSourceControlOperationComplete::CreateRaw(this, &FPlasticSourceControlMenu::OnSourceControlOperationComplete));
		if (Result == ECommandResult::Succeeded)
		{
			// Display an ongoing notification during the whole operation
			DisplayInProgressNotification(RefreshOperation->GetInProgressString());
		}
		else
		{
			// Report failure with a notification
			DisplayFailureNotification(RefreshOperation->GetName());
		}
	}
	else
	{
		FMessageLog SourceControlLog("SourceControl");
		SourceControlLog.Warning(LOCTEXT("SourceControlMenu_InProgress", "Source control operation already in progress"));
		SourceControlLog.Notify();
	}
}

void FPlasticSourceControlMenu::SwitchToPartialWorkspaceClicked()
{
	if (!OperationInProgressNotification.IsValid())
	{
		// Ask the user before switching to Partial Workspace. It's not possible to switch back with local changes!
		const FText DialogText(LOCTEXT("SourceControlMenu_AskSwitchToPartialWorkspace", "Switch to Gluon partial workspace?\n"
			"Please note that, in order to switch back to a regular workspace you will need to undo all local changes."));
		const EAppReturnType::Type Choice = FMessageDialog::Open(EAppMsgType::OkCancel, DialogText);
		if (Choice == EAppReturnType::Ok)
		{
			// Launch a "SwitchToPartialWorkspace" Operation
			FPlasticSourceControlProvider& Provider = FPlasticSourceControlModule::Get().GetProvider();
			TSharedRef<FPlasticSwitchToPartialWorkspace, ESPMode::ThreadSafe> SwitchOperation = ISourceControlOperation::Create<FPlasticSwitchToPartialWorkspace>();
			const ECommandResult::Type Result = Provider.Execute(SwitchOperation, TArray<FString>(), EConcurrency::Asynchronous, FSourceControlOperationComplete::CreateRaw(this, &FPlasticSourceControlMenu::OnSourceControlOperationComplete));
			if (Result == ECommandResult::Succeeded)
			{
				// Display an ongoing notification during the whole operation
				DisplayInProgressNotification(SwitchOperation->GetInProgressString());
			}
			else
			{
				// Report failure with a notification
				DisplayFailureNotification(SwitchOperation->GetName());
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

bool FPlasticSourceControlMenu::CanSwitchToPartialWorkspace() const
{
	return !FPlasticSourceControlModule::Get().GetProvider().IsPartialWorkspace();
}

void FPlasticSourceControlMenu::ShowSourceControlEditorPreferences() const
{
	if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
	{
		SettingsModule->ShowViewer("Editor", "General", "LoadingSaving");
	}
}

void FPlasticSourceControlMenu::ShowSourceControlProjectSettings() const
{
	if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
	{
		SettingsModule->ShowViewer("Project", "Editor", "SourceControlPreferences");
	}
}

void FPlasticSourceControlMenu::ShowSourceControlPlasticScmProjectSettings() const
{
	if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
	{
		SettingsModule->ShowViewer("Project", "Editor", "PlasticSourceControlProjectSettings");
	}
}

void FPlasticSourceControlMenu::VisitDocsURLClicked() const
{
	// Grab the URL from the uplugin file
	const TSharedPtr<IPlugin> Plugin = FPlasticSourceControlModule::GetPlugin();
	if (Plugin.IsValid())
	{
		FPlatformProcess::LaunchURL(*Plugin->GetDescriptor().DocsURL, NULL, NULL);
	}
}

void FPlasticSourceControlMenu::VisitSupportURLClicked() const
{
	// Grab the URL from the uplugin file
	const TSharedPtr<IPlugin> Plugin = FPlasticSourceControlModule::GetPlugin();
	if (Plugin.IsValid())
	{
		FPlatformProcess::LaunchURL(*Plugin->GetDescriptor().SupportURL, NULL, NULL);
	}
}

void FPlasticSourceControlMenu::VisitLockRulesURLClicked(const FString InOrganizationName) const
{
	const FString OrganizationLockRulesURL = FString::Printf(
		TEXT("https://dashboard.unity3d.com/devops/organizations/default/plastic-scm/organizations/%s/lock-rules"),
		*InOrganizationName
	);
	FPlatformProcess::LaunchURL(*OrganizationLockRulesURL, NULL, NULL);
}

void FPlasticSourceControlMenu::OpenBranchesWindow() const
{
	FPlasticSourceControlModule::Get().GetUnityVersionControlWindow().OpenTab();
}

// Display an ongoing notification during the whole operation
void FPlasticSourceControlMenu::DisplayInProgressNotification(const FText& InOperationInProgressString)
{
	if (!OperationInProgressNotification.IsValid())
	{
		FNotificationInfo Info(InOperationInProgressString);
		Info.bFireAndForget = false;
		Info.ExpireDuration = 0.0f;
		Info.FadeOutDuration = 1.0f;
		OperationInProgressNotification = FSlateNotificationManager::Get().AddNotification(Info);
		if (OperationInProgressNotification.IsValid())
		{
			OperationInProgressNotification.Pin()->SetCompletionState(SNotificationItem::CS_Pending);
		}
	}
}

// Remove the ongoing notification at the end of the operation
void FPlasticSourceControlMenu::RemoveInProgressNotification()
{
	if (OperationInProgressNotification.IsValid())
	{
		OperationInProgressNotification.Pin()->ExpireAndFadeout();
		OperationInProgressNotification.Reset();
	}
}

// Display a temporary success notification at the end of the operation
void FPlasticSourceControlMenu::DisplaySucessNotification(const FName& InOperationName)
{
	const FText NotificationText = FText::Format(
		LOCTEXT("SourceControlMenu_Success", "{0} operation was successful!"),
		FText::FromName(InOperationName)
	);
	FNotificationInfo Info(NotificationText);
	Info.bUseSuccessFailIcons = true;
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
	Info.Image = FAppStyle::GetBrush(TEXT("NotificationList.SuccessImage"));
#else
	Info.Image = FEditorStyle::GetBrush(TEXT("NotificationList.SuccessImage"));
#endif
	FSlateNotificationManager::Get().AddNotification(Info);
	UE_LOG(LogSourceControl, Verbose, TEXT("%s"), *NotificationText.ToString());
}

// Display a temporary failure notification at the end of the operation
void FPlasticSourceControlMenu::DisplayFailureNotification(const FName& InOperationName)
{
	const FText NotificationText = FText::Format(
		LOCTEXT("SourceControlMenu_Failure", "Error: {0} operation failed!"),
		FText::FromName(InOperationName)
	);
	FNotificationInfo Info(NotificationText);
	Info.ExpireDuration = 8.0f;
	FSlateNotificationManager::Get().AddNotification(Info);
	UE_LOG(LogSourceControl, Error, TEXT("%s"), *NotificationText.ToString());
}

void FPlasticSourceControlMenu::OnSyncAllOperationComplete(const FSourceControlOperationRef& InOperation, ECommandResult::Type InResult)
{
	OnSourceControlOperationComplete(InOperation, InResult);

	// Reload packages that where updated by the Sync operation (and the current map if needed)
	TSharedRef<FPlasticSyncAll, ESPMode::ThreadSafe> Operation = StaticCastSharedRef<FPlasticSyncAll>(InOperation);
	PackageUtils::ReloadPackages(Operation->UpdatedFiles);
}

void FPlasticSourceControlMenu::OnRevertAllOperationComplete(const FSourceControlOperationRef & InOperation, ECommandResult::Type InResult)
{
	OnSourceControlOperationComplete(InOperation, InResult);

	// Reload packages that where updated by the Revert operation (and the current map if needed)
	TSharedRef<FPlasticRevertAll, ESPMode::ThreadSafe> Operation = StaticCastSharedRef<FPlasticRevertAll>(InOperation);
	PackageUtils::ReloadPackages(Operation->UpdatedFiles);
}

void FPlasticSourceControlMenu::OnSourceControlOperationComplete(const FSourceControlOperationRef& InOperation, ECommandResult::Type InResult)
{
	RemoveInProgressNotification();

	// Report result with a notification
	if (InResult == ECommandResult::Succeeded)
	{
		DisplaySucessNotification(InOperation->GetName());
	}
	else
	{
		DisplayFailureNotification(InOperation->GetName());
	}
}

#if ENGINE_MAJOR_VERSION == 4
void FPlasticSourceControlMenu::AddMenuActions(FMenuBuilder& Menu)
#elif ENGINE_MAJOR_VERSION == 5
void FPlasticSourceControlMenu::AddMenuActions(FToolMenuSection& Menu)
#endif
{
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
			FExecuteAction::CreateRaw(this, &FPlasticSourceControlMenu::SyncProjectClicked),
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
			FExecuteAction::CreateRaw(this, &FPlasticSourceControlMenu::RevertUnchangedClicked),
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
			FExecuteAction::CreateRaw(this, &FPlasticSourceControlMenu::RevertAllClicked),
			FCanExecuteAction()
		)
	);

	Menu.AddMenuEntry(
#if ENGINE_MAJOR_VERSION == 5
		"PlasticRefresh",
#endif
		LOCTEXT("PlasticRefresh",			"Refresh"),
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 2
		LOCTEXT("PlasticRefreshTooltip",	"Update the local revision control status of all files in the workspace (no expensive checks for locks or changes on other branches)."),
#else
		LOCTEXT("PlasticRefreshTooltip",	"Update the local source control status of all files in the workspace (no expensive checks for locks or changes on other branches)."),
#endif
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "SourceControl.Actions.Refresh"),
#else
		FSlateIcon(FEditorStyle::GetStyleSetName(), "SourceControl.Actions.Refresh"),
#endif
		FUIAction(
			FExecuteAction::CreateRaw(this, &FPlasticSourceControlMenu::RefreshClicked),
			FCanExecuteAction()
		)
	);


	// TODO add "Recent branches" as a submenu?
	// can we fit a full submenu on each of these branches? Is it even a good UI/UX practice (I doubt it)
	//   - switch to?
	//   - delete?
	//   - rename?
	// TODO add "Create branch"
	Menu.AddMenuEntry(
#if ENGINE_MAJOR_VERSION == 5
		TEXT("PlasticBranchesWindow"),
#endif
		LOCTEXT("PlasticBranchesWindow", "Show Branches"),
		LOCTEXT("PlasticBranchesWindowTooltip", "Open the Branches Window."),
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "SourceControl.Branch"),
#elif ENGINE_MAJOR_VERSION == 5
		FSlateIcon(FEditorStyle::GetStyleSetName(), "SourceControl.Branch"),
#elif ENGINE_MAJOR_VERSION == 4
		FSlateIcon(FEditorStyle::GetStyleSetName(), "SourceControl.Branch"),
#endif
		FUIAction(
			FExecuteAction::CreateRaw(this, &FPlasticSourceControlMenu::OpenBranchesWindow),
			FCanExecuteAction()
		)
	);
}


void FPlasticSourceControlMenu::GeneratePlasticSettingsMenu(FMenuBuilder& MenuBuilder)
{
	MenuBuilder.BeginSection("PlasticSettings", LOCTEXT("PlasticSettingsSubMenuHeading", "Unity Version Control Settings"));

	{
		MenuBuilder.AddMenuEntry(
			LOCTEXT("SourceControlEditorPreferences", "Editor Preferences - Source Control"),
			LOCTEXT("SourceControlEditorPreferencesTooltip", "Open the Load & Save section with Source Control in the Editor Preferences."),
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "EditorPreferences.TabIcon"),
#else
			FSlateIcon(FEditorStyle::GetStyleSetName(), "EditorPreferences.TabIcon"),
#endif
			FUIAction(
				FExecuteAction::CreateRaw(this, &FPlasticSourceControlMenu::ShowSourceControlEditorPreferences),
				FCanExecuteAction()
			)
		);

#if ENGINE_MAJOR_VERSION == 5 // Disable the "Source Control Project Settings" for UE4 since this section is new to UE5
		MenuBuilder.AddMenuEntry(
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 2
			LOCTEXT("SourceControlProjectSettings", "Project Settings - Editor - Revision Control"),
			LOCTEXT("SourceControlProjectSettingsTooltip", "Open the Revision Control section in the Project Settings."),
#else
			LOCTEXT("SourceControlProjectSettings", "Project Settings - Editor - Source Control"),
			LOCTEXT("SourceControlProjectSettingsTooltip", "Open the Source Control section in the Project Settings."),
#endif
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "ProjectSettings.TabIcon"),
#else
			FSlateIcon(FEditorStyle::GetStyleSetName(), "ProjectSettings.TabIcon"),
#endif
			FUIAction(
				FExecuteAction::CreateRaw(this, &FPlasticSourceControlMenu::ShowSourceControlProjectSettings),
				FCanExecuteAction()
			)
		);
#endif

		MenuBuilder.AddMenuEntry(
			LOCTEXT("PlasticProjectSettings", "Project Settings - Editor - Source Control - Unity Version Control"),
			LOCTEXT("PlasticProjectSettingsTooltip", "Open the Unity Version Control (formerly Plastic SCM) section in the Project Settings."),
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "ProjectSettings.TabIcon"),
#else
			FSlateIcon(FEditorStyle::GetStyleSetName(), "ProjectSettings.TabIcon"),
#endif
			FUIAction(
				FExecuteAction::CreateRaw(this, &FPlasticSourceControlMenu::ShowSourceControlPlasticScmProjectSettings),
				FCanExecuteAction()
			)
		);
	}

	MenuBuilder.EndSection();
}

void FPlasticSourceControlMenu::GeneratePlasticWebLinksMenu(FMenuBuilder& MenuBuilder)
{
	MenuBuilder.BeginSection("PlasticSettings", LOCTEXT("PlasticSettingsSubMenuHeading", "Unity Version Control Settings"));

	{
		MenuBuilder.AddMenuEntry(
			LOCTEXT("PlasticDocsURL", "Plugin's Documentation"),
			LOCTEXT("PlasticDocsURLTooltip", "Visit documentation of the plugin on Github."),
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Documentation"),
#elif ENGINE_MAJOR_VERSION == 5
			FSlateIcon(FEditorStyle::GetStyleSetName(), "Icons.Documentation"),
#elif ENGINE_MAJOR_VERSION == 4
			FSlateIcon(FEditorStyle::GetStyleSetName(), "LevelEditor.BrowseDocumentation"),
#endif
			FUIAction(
				FExecuteAction::CreateRaw(this, &FPlasticSourceControlMenu::VisitDocsURLClicked),
				FCanExecuteAction()
			)
		);

		MenuBuilder.AddMenuEntry(
			LOCTEXT("PlasticSupportURL", "Unity Version Control Support"),
			LOCTEXT("PlasticSupportURLTooltip", "Submit a support request for Unity Version Control (formerly Plastic SCM)."),
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Support"),
#elif ENGINE_MAJOR_VERSION == 5
			FSlateIcon(FEditorStyle::GetStyleSetName(), "Icons.Support"),
#elif ENGINE_MAJOR_VERSION == 4
			FSlateIcon(FEditorStyle::GetStyleSetName(), "LevelEditor.BrowseDocumentation"),
#endif
			FUIAction(
				FExecuteAction::CreateRaw(this, &FPlasticSourceControlMenu::VisitSupportURLClicked),
				FCanExecuteAction()
			)
		);
	}

	MenuBuilder.EndSection();
}

void FPlasticSourceControlMenu::GeneratePlasticAdvancedMenu(FMenuBuilder& MenuBuilder)
{
	// TODO: we should offer a switch back, even though it has more constraints (requires reverting or shelving all changes)
	MenuBuilder.AddMenuEntry(
		LOCTEXT("SwitchToPartialWorkspace", "Switch to Gluon Partial Workspace"),
		LOCTEXT("SwitchToPartialWorkspaceTooltip", "Update the workspace to a Gluon partial mode for a simplified workflow.\n"
			"Allows to update and check in files individually as opposed to the whole workspace.\nIt doesn't work with branches or shelves."),
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "GenericCommands.Cut"),
#else
		FSlateIcon(FEditorStyle::GetStyleSetName(), "GenericCommands.Cut"),
#endif
		FUIAction(
			FExecuteAction::CreateRaw(this, &FPlasticSourceControlMenu::SwitchToPartialWorkspaceClicked),
			FCanExecuteAction::CreateRaw(this, &FPlasticSourceControlMenu::CanSwitchToPartialWorkspace)
		)
	);

	FString OrganizationName = FPlasticSourceControlModule::Get().GetProvider().GetCloudOrganization();
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
				FExecuteAction::CreateRaw(this, &FPlasticSourceControlMenu::VisitLockRulesURLClicked, MoveTemp(OrganizationName)),
				FCanExecuteAction()
			)
		);
	}
}


#if ENGINE_MAJOR_VERSION == 4
TSharedRef<FExtender> FPlasticSourceControlMenu::OnExtendLevelEditorViewMenu(const TSharedRef<FUICommandList> CommandList)
{
	TSharedRef<FExtender> Extender(new FExtender());

	Extender->AddMenuExtension(
		"SourceControlActions",
		EExtensionHook::After,
		nullptr,
		FMenuExtensionDelegate::CreateRaw(this, &FPlasticSourceControlMenu::AddMenuExtension));

	return Extender;
}
#endif



void SPlasticSourceControlStatusBar::Construct(const FArguments& InArgs)
{
	ChildSlot
	[
		SNew(SButton)
		.ContentPadding(FMargin(6.0f, 0.0f))
		.ToolTipText_Lambda([this]() { return GetStatusBarTooltip(); })
		.ButtonStyle(FAppStyle::Get(), "SimpleButton")
		.OnClicked(this, &SPlasticSourceControlStatusBar::OnClicked)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Center)
			[
				SNew(SImage)
				.Image_Lambda([this](){ return GetStatusBarIcon(); })
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(FMargin(5, 0, 0, 0))
			[
				SNew(STextBlock)
				.TextStyle(&FAppStyle::Get().GetWidgetStyle<FTextBlockStyle>("NormalText"))
				.Text_Lambda([this]() { return GetStatusBarText(); })
			]
		]
	];
}

const FSlateBrush* SPlasticSourceControlStatusBar::GetStatusBarIcon() const
{
	// TODO plastic log? branch logo?
	return FAppStyle::GetBrush("SourceControl.Branch");
}

FText SPlasticSourceControlStatusBar::GetStatusBarText() const
{;
	return FText::FromString(FPlasticSourceControlModule::Get().GetProvider().GetBranchName());
}

FText SPlasticSourceControlStatusBar::GetStatusBarTooltip() const
{
	return LOCTEXT("Branches_Tooltip", "Open Window to manage branches");
}


FReply SPlasticSourceControlStatusBar::OnClicked()
{
	FPlasticSourceControlModule::Get().GetUnityVersionControlWindow().OpenTab();

	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE

#endif
