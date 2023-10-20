// Copyright (c) 2023 Unity Technologies

#pragma once

#include "CoreMinimal.h"
#include "ISourceControlProvider.h"
#include "Runtime/Launch/Resources/Version.h"

#if SOURCE_CONTROL_WITH_SLATE

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
	// TODO UE4.27
	void AddMenuExtension(FMenuBuilder& Menu);

	TSharedRef<class FExtender> OnExtendLevelEditorViewMenu(const TSharedRef<class FUICommandList> CommandList);
#elif ENGINE_MAJOR_VERSION == 5
	void AddMenuActions(FToolMenuSection& Menu);
#endif

	// TODO POC
	/** Called to generate sub menus. */
	void GeneratePlasticSettingsMenu(FMenuBuilder& MenuBuilder);
	void GeneratePlasticWebLinksMenu(FMenuBuilder& MenuBuilder);
	void GeneratePlasticAdvancedMenu(FMenuBuilder& MenuBuilder);
	void GeneratePlasticBranchesMenu(FMenuBuilder& MenuBuilder);


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

	/** Delegates called when a source control operation has completed */
	void OnSyncAllOperationComplete(const FSourceControlOperationRef& InOperation, ECommandResult::Type InResult);
	void OnRevertAllOperationComplete(const FSourceControlOperationRef& InOperation, ECommandResult::Type InResult);
	/** Generic delegate and notification handler */
	void OnSourceControlOperationComplete(const FSourceControlOperationRef& InOperation, ECommandResult::Type InResult);
};


// TODO POC move this to a separate file, with a different name etc!

class SPlasticSourceControlBranches : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SPlasticSourceControlBranches) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:

};

/**
 * Tracks assets that has in-memory modification not saved to disk yet and checks
 * the source control states of those assets when a source control provider is available.
 */
class SPlasticSourceControlStatusBar : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SPlasticSourceControlStatusBar)	{}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	const FSlateBrush* GetStatusBarIcon() const;
	FText GetStatusBarText() const;
	FText GetStatusBarTooltip() const;

	FReply OnClicked();

	/** Delegate called when the source control window is closed */
	void OnSourceControlDialogClosed(const TSharedRef<class SWindow>& InWindow);

private:
	// TODO even more POC from FSourceControlModule

	/** The login window we may be using */
	TSharedPtr<SWindow> PlasticSourceControlBranchesWindowPtr;

	/** The login window control we may be using */
	TSharedPtr<class SPlasticSourceControlBranches> PlasticSourceControlBranchesContentPtr;
};

#endif
