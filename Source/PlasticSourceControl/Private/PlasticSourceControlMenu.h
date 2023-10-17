// Copyright (c) 2023 Unity Technologies

#pragma once

#include "CoreMinimal.h"
#include "ISourceControlProvider.h"
#include "Runtime/Launch/Resources/Version.h"

class FMenuBuilder;
struct FToolMenuSection;

/** Unity Version Control extension of the Source Control toolbar menu */
class FPlasticSourceControlMenu
{
public:
	void Register();
	void Unregister();

	/** This functions will be bound to appropriate Command. */
	void SyncProjectClicked();
	void RevertUnchangedClicked();
	void RevertAllClicked();
	void RefreshClicked();
	void SwitchToPartialWorkspaceClicked();
	bool CanSwitchToPartialWorkspace() const;
	void ShowSourceControlEditorPreferences() const;
	void ShowSourceControlProjectSettings() const;
	void ShowSourceControlPlasticScmProjectSettings() const;
	void VisitDocsURLClicked() const;
	void VisitSupportURLClicked() const;
	void VisitLockRulesURLClicked(const FString InOrganizationName) const;

private:
	bool IsSourceControlConnected() const;

	bool				SaveDirtyPackages();
	TArray<FString>		ListAllPackages();

#if ENGINE_MAJOR_VERSION == 4
	void AddMenuExtension(FMenuBuilder& Menu);

	TSharedRef<class FExtender> OnExtendLevelEditorViewMenu(const TSharedRef<class FUICommandList> CommandList);
#elif ENGINE_MAJOR_VERSION == 5
	void AddMenuExtension(FToolMenuSection& Menu);
#endif

	// TODO REVIEW POC to be renamed and reworked as needed

	/** Extends the toolbar with MU source control options */
	void ExtendToolbarWithSourceControlMenu();
	TSharedRef<SWidget> CreateStatusBarWidget();

	/** Extends the main Revision Control menu from the toolbar at the bottom-right. */
	void ExtendRevisionControlMenu();
	/** Extends the content browser asset context menu with Admin revision control options. */
	void ExtendAssetContextMenu();
	/** Called to generate concert asset context menu. */
	void GeneratePlasticAssetContextMenu(FMenuBuilder& MenuBuilder, TArray<FAssetData> InAssetObjectPaths);

	bool CanRemoveLocks(TArray<FAssetData> InAssetObjectPaths) const;
	bool CanReleaseLocks(TArray<FAssetData> InAssetObjectPaths) const;
	void ExecuteRemoveLocks(TArray<FAssetData> InAssetObjectPaths);
	void ExecuteReleaseLocks(TArray<FAssetData> InAssetObjectPaths);
	void ExecuteUnlock(const TArray<FAssetData>& InAssetObjectPaths, const bool bInRemove);

	void DisplayInProgressNotification(const FText& InOperationInProgressString);
	void RemoveInProgressNotification();
	void DisplaySucessNotification(const FName& InOperationName);
	void DisplayFailureNotification(const FName& InOperationName);

private:
	/** Tracks if the menu extension has been registered with the editor or not */
	bool bHasRegistered = false;

#if ENGINE_MAJOR_VERSION == 4
	FDelegateHandle ViewMenuExtenderHandle;
#endif

	/** Current source control operation from extended menu if any */
	TWeakPtr<class SNotificationItem> OperationInProgressNotification;

	/** Name of the menu extension going into the global Revision Control (on the toolbar at the bottom-right) */
	static FName UnityVersionControlMainMenuOwnerName;
	/** Name of the asset context menu extension for admin actions over Locks */
	static FName UnityVersionControlAssetContextLocksMenuOwnerName;

	/** Delegates called when a source control operation has completed */
	void OnSyncAllOperationComplete(const FSourceControlOperationRef& InOperation, ECommandResult::Type InResult);
	void OnRevertAllOperationComplete(const FSourceControlOperationRef& InOperation, ECommandResult::Type InResult);
	/** Generic delegate and notification handler */
	void OnSourceControlOperationComplete(const FSourceControlOperationRef& InOperation, ECommandResult::Type InResult);
};


// TODO move this to a separate file, with a different name etc!

/**
 * Tracks assets that has in-memory modification not saved to disk yet and checks
 * the source control states of those assets when a source control provider is available.
 */
class SUnsavedAssetsStatusBarWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SUnsavedAssetsStatusBarWidget)
	{}
	/** Event fired when the status bar button is clicked */
	SLATE_EVENT(FOnClicked, OnClicked)
		SLATE_END_ARGS()

		/** Constructs the widget */
		void Construct(const FArguments& InArgs);

private:
	const FSlateBrush* GetStatusBarIcon() const;
	FText GetStatusBarText() const;
	FText GetStatusBarTooltip() const;

private:
};

