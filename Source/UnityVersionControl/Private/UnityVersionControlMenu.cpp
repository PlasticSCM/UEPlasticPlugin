// Copyright (c) 2023 Unity Technologies

#include "UnityVersionControlMenu.h"

#include "UnityVersionControlModule.h"
#include "UnityVersionControlProvider.h"
#include "UnityVersionControlOperations.h"

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

#define LOCTEXT_NAMESPACE "UnityVersionControl"

FName FUnityVersionControlMenu::UnityVersionControlMainMenuOwnerName = TEXT("UnityVersionControlMenu");
FName FUnityVersionControlMenu::UnityVersionControlAssetContextLocksMenuOwnerName = TEXT("UnityVersionControlContextLocksMenu");

void FUnityVersionControlMenu::Register()
{
	if (bHasRegistered)
	{
		return;
	}

	// Register the menu extensions with the level editor
	ExtendRevisionControlMenu();
	ExtendAssetContextMenu();
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
		bHasRegistered = false;
	}
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
	if (UToolMenus* ToolMenus = UToolMenus::Get())
	{
		if (UToolMenu* SourceControlMenu = ToolMenus->ExtendMenu("StatusBar.ToolBar.SourceControl"))
		{
			FToolMenuSection& Section = SourceControlMenu->AddSection("UnityVersionControlActions", LOCTEXT("UnityVersionControlMenuHeadingActions", "Unity Version Control"), FToolMenuInsert(NAME_None, EToolMenuInsertType::First));

			AddMenuExtension(Section);

			bHasRegistered = true;
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
				FExecuteAction::CreateRaw(this, &FUnityVersionControlMenu::ExecuteReleaseLocks, InAssetObjectPaths),
				FCanExecuteAction::CreateRaw(this, &FUnityVersionControlMenu::CanReleaseLocks, InAssetObjectPaths)
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
				FExecuteAction::CreateRaw(this, &FUnityVersionControlMenu::ExecuteRemoveLocks, InAssetObjectPaths),
				FCanExecuteAction::CreateRaw(this, &FUnityVersionControlMenu::CanRemoveLocks, InAssetObjectPaths)
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

bool FUnityVersionControlMenu::CanReleaseLocks(TArray<FAssetData> InAssetObjectPaths) const
{
	const TArray<FString> Files = PackageUtils::AssetDateToFileNames(InAssetObjectPaths);

	for (const FString& File : Files)
	{
		const FString AbsoluteFilename = FPaths::ConvertRelativePathToFull(File);
		const auto State = FUnityVersionControlModule::Get().GetProvider().GetStateInternal(AbsoluteFilename);
		// If exclusively Checked Out (Locked) the lock can be released coming back to it's potential underlying "Retained" status if changes where already checked in the branch
		if (!State->LockedBy.IsEmpty() && State->LockedId != ISourceControlState::INVALID_REVISION)
		{
			return true;
		}
	}

	return false;
}

bool FUnityVersionControlMenu::CanRemoveLocks(TArray<FAssetData> InAssetObjectPaths) const
{
	const TArray<FString> Files = PackageUtils::AssetDateToFileNames(InAssetObjectPaths);

	for (const FString& File : Files)
	{
		const FString AbsoluteFilename = FPaths::ConvertRelativePathToFull(File);
		const auto State = FUnityVersionControlModule::Get().GetProvider().GetStateInternal(AbsoluteFilename);
		// If Locked or Retained, the lock can be removed, that is completely deleted in order to simply ignore the changes from the branch
		if (State->LockedId != ISourceControlState::INVALID_REVISION)
		{
			return true;
		}
	}

	return false;
}

void FUnityVersionControlMenu::ExecuteReleaseLocks(TArray<FAssetData> InAssetObjectPaths)
{
	ExecuteUnlock(InAssetObjectPaths, false);
}

void FUnityVersionControlMenu::ExecuteRemoveLocks(TArray<FAssetData> InAssetObjectPaths)
{
	ExecuteUnlock(InAssetObjectPaths, true);
}

void FUnityVersionControlMenu::ExecuteUnlock(const TArray<FAssetData>& InAssetObjectPaths, const bool bInRemove)
{
	if (!OperationInProgressNotification.IsValid())
	{
		const TArray<FString> Files = PackageUtils::AssetDateToFileNames(InAssetObjectPaths);

		// Launch a custom "Release/Remove Lock" operation
		FUnityVersionControlProvider& Provider = FUnityVersionControlModule::Get().GetProvider();
		TSharedRef<FPlasticUnlock, ESPMode::ThreadSafe> UnlockOperation = ISourceControlOperation::Create<FPlasticUnlock>();
		const ECommandResult::Type Result = Provider.Execute(UnlockOperation, Files, EConcurrency::Asynchronous, FSourceControlOperationComplete::CreateRaw(this, &FUnityVersionControlMenu::OnSourceControlOperationComplete));
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

bool FUnityVersionControlMenu::IsSourceControlConnected() const
{
	const ISourceControlProvider& Provider = ISourceControlModule::Get().GetProvider();
	return Provider.IsEnabled() && Provider.IsAvailable();
}

/// Prompt to save or discard all packages
bool FUnityVersionControlMenu::SaveDirtyPackages()
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
TArray<FString> FUnityVersionControlMenu::ListAllPackages()
{
	TArray<FString> PackageFilePaths;
	FPackageName::FindPackagesInDirectory(PackageFilePaths, FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir()));
	return PackageFilePaths;
}

void FUnityVersionControlMenu::SyncProjectClicked()
{
	if (!OperationInProgressNotification.IsValid())
	{
		const bool bSaved = SaveDirtyPackages();
		if (bSaved)
		{
			// Find and Unlink all loaded packages in Content directory to allow to update them
			PackageUtils::UnlinkPackages(ListAllPackages());

			// Launch a custom "SyncAll" operation
			FUnityVersionControlProvider& Provider = FUnityVersionControlModule::Get().GetProvider();
			TSharedRef<FPlasticSyncAll, ESPMode::ThreadSafe> SyncOperation = ISourceControlOperation::Create<FPlasticSyncAll>();
			const ECommandResult::Type Result = Provider.Execute(SyncOperation, TArray<FString>(), EConcurrency::Asynchronous, FSourceControlOperationComplete::CreateRaw(this, &FUnityVersionControlMenu::OnSyncAllOperationComplete));
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

void FUnityVersionControlMenu::RevertUnchangedClicked()
{
	if (!OperationInProgressNotification.IsValid())
	{
		// Launch a "RevertUnchanged" Operation
		FUnityVersionControlProvider& Provider = FUnityVersionControlModule::Get().GetProvider();
		TSharedRef<FPlasticRevertUnchanged, ESPMode::ThreadSafe> RevertUnchangedOperation = ISourceControlOperation::Create<FPlasticRevertUnchanged>();
		const ECommandResult::Type Result = Provider.Execute(RevertUnchangedOperation, TArray<FString>(), EConcurrency::Asynchronous, FSourceControlOperationComplete::CreateRaw(this, &FUnityVersionControlMenu::OnSourceControlOperationComplete));
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

void FUnityVersionControlMenu::RevertAllClicked()
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
				FUnityVersionControlProvider& Provider = FUnityVersionControlModule::Get().GetProvider();
				TSharedRef<FPlasticRevertAll, ESPMode::ThreadSafe> RevertAllOperation = ISourceControlOperation::Create<FPlasticRevertAll>();
				const ECommandResult::Type Result = Provider.Execute(RevertAllOperation, TArray<FString>(), EConcurrency::Asynchronous, FSourceControlOperationComplete::CreateRaw(this, &FUnityVersionControlMenu::OnRevertAllOperationComplete));
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

void FUnityVersionControlMenu::RefreshClicked()
{
	if (!OperationInProgressNotification.IsValid())
	{
		// Launch an "UpdateStatus" Operation
		FUnityVersionControlProvider& Provider = FUnityVersionControlModule::Get().GetProvider();
		TSharedRef<FUpdateStatus, ESPMode::ThreadSafe> RefreshOperation = ISourceControlOperation::Create<FUpdateStatus>();
		// This is the flag used by the Content Browser's "Checkout" filter to trigger a full status update
		RefreshOperation->SetGetOpenedOnly(true);
		const ECommandResult::Type Result = Provider.Execute(RefreshOperation, TArray<FString>(), EConcurrency::Asynchronous, FSourceControlOperationComplete::CreateRaw(this, &FUnityVersionControlMenu::OnSourceControlOperationComplete));
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

void FUnityVersionControlMenu::SwitchToPartialWorkspaceClicked()
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
			FUnityVersionControlProvider& Provider = FUnityVersionControlModule::Get().GetProvider();
			TSharedRef<FPlasticSwitchToPartialWorkspace, ESPMode::ThreadSafe> SwitchOperation = ISourceControlOperation::Create<FPlasticSwitchToPartialWorkspace>();
			const ECommandResult::Type Result = Provider.Execute(SwitchOperation, TArray<FString>(), EConcurrency::Asynchronous, FSourceControlOperationComplete::CreateRaw(this, &FUnityVersionControlMenu::OnSourceControlOperationComplete));
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
	const FString OrganizationLockRulesURL = FString::Printf(
		TEXT("https://dashboard.unity3d.com/devops/organizations/default/plastic-scm/organizations/%s/lock-rules"),
		*InOrganizationName
	);
	FPlatformProcess::LaunchURL(*OrganizationLockRulesURL, NULL, NULL);
}

// Display an ongoing notification during the whole operation
void FUnityVersionControlMenu::DisplayInProgressNotification(const FText& InOperationInProgressString)
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
void FUnityVersionControlMenu::RemoveInProgressNotification()
{
	if (OperationInProgressNotification.IsValid())
	{
		OperationInProgressNotification.Pin()->ExpireAndFadeout();
		OperationInProgressNotification.Reset();
	}
}

// Display a temporary success notification at the end of the operation
void FUnityVersionControlMenu::DisplaySucessNotification(const FName& InOperationName)
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
void FUnityVersionControlMenu::DisplayFailureNotification(const FName& InOperationName)
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
void FUnityVersionControlMenu::AddMenuExtension(FMenuBuilder& Menu)
#elif ENGINE_MAJOR_VERSION == 5
void FUnityVersionControlMenu::AddMenuExtension(FToolMenuSection& Menu)
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
			FExecuteAction::CreateRaw(this, &FUnityVersionControlMenu::RefreshClicked),
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
