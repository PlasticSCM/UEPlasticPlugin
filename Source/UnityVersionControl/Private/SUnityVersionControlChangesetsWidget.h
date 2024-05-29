// Copyright (c) 2024 Unity Technologies

#pragma once

#include "CoreMinimal.h"

#include "Notification.h"

#include "Misc/TextFilter.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/SListView.h"

#include "ISourceControlOperation.h"
#include "ISourceControlProvider.h"

class FUnityVersionControlChangeset;
class FUnityVersionControlState;
typedef TSharedRef<class FUnityVersionControlChangeset, ESPMode::ThreadSafe> FUnityVersionControlChangesetRef;
typedef TSharedPtr<class FUnityVersionControlChangeset, ESPMode::ThreadSafe> FUnityVersionControlChangesetPtr;
typedef TSharedRef<class FUnityVersionControlState, ESPMode::ThreadSafe> FUnityVersionControlStateRef;
typedef TSharedPtr<class FUnityVersionControlState, ESPMode::ThreadSafe> FUnityVersionControlStatePtr;

class SSearchBox;

// Widget displaying the list of Changesets in the tab window, see FUnityVersionControlChangesetsWindow
class SUnityVersionControlChangesetsWidget : public SCompoundWidget
{
	SLATE_BEGIN_ARGS(SUnityVersionControlChangesetsWidget) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

private:
	TSharedRef<SWidget> CreateToolBar();
	TSharedRef<SWidget> CreateChangesetsListView();
	TSharedRef<SWidget> CreateFilesListView();

	TSharedRef<ITableRow> OnGenerateRow(FUnityVersionControlChangesetRef InChangeset, const TSharedRef<STableViewBase>& OwnerTable);
	TSharedRef<ITableRow> OnGenerateRow(FUnityVersionControlStateRef InChangeset, const TSharedRef<STableViewBase>& OwnerTable);
	void OnHiddenColumnsListChanged();

	void OnChangesetsSearchTextChanged(const FText& InFilterText);
	void OnFilesSearchTextChanged(const FText& InFilterText);
	void PopulateItemSearchStrings(const FUnityVersionControlChangeset& InItem, TArray<FString>& OutStrings);
	void PopulateItemSearchStrings(const FUnityVersionControlState& InItem, TArray<FString>& OutStrings);

	TSharedRef<SWidget> BuildFromDateDropDownMenu();
	void OnFromDateChanged(int32 InFromDateInDays);

	void OnChangesetsRefreshUI();
	void OnFilesRefreshUI();

	EColumnSortPriority::Type GetChangesetsColumnSortPriority(const FName InColumnId) const;
	EColumnSortMode::Type GetChangesetsColumnSortMode(const FName InColumnId) const;
	void OnChangesetsColumnSortModeChanged(const EColumnSortPriority::Type InSortPriority, const FName& InColumnId, const EColumnSortMode::Type InSortMode);

	EColumnSortPriority::Type GetFilesColumnSortPriority(const FName InColumnId) const;
	EColumnSortMode::Type GetFilesColumnSortMode(const FName InColumnId) const;
	void OnFilesColumnSortModeChanged(const EColumnSortPriority::Type InSortPriority, const FName& InColumnId, const EColumnSortMode::Type InSortMode);

	void SortChangesetsView();
	void SortFilesView();

	TSharedPtr<SWidget> OnOpenChangesetContextMenu();
	TSharedPtr<SWidget> OnOpenFileContextMenu();

	void OnDiffChangesetClicked(FUnityVersionControlChangesetPtr InSelectedChangeset);
	void OnDiffChangesetsClicked(TArray<FUnityVersionControlChangesetRef> InSelectedChangesets);
	void OnDiffBranchClicked(FUnityVersionControlChangesetPtr InSelectedChangeset);
	void OnSwitchToBranchClicked(FUnityVersionControlChangesetPtr InSelectedChangeset);
	void OnSwitchToChangesetClicked(FUnityVersionControlChangesetPtr InSelectedChangeset);
	void OnLocateFileClicked(FUnityVersionControlStateRef InSelectedFile);
	void OnDiffRevisionClicked(FUnityVersionControlStateRef InSelectedFile);
	void OnDiffAgainstWorkspaceClicked(FUnityVersionControlStateRef InSelectedFile);
	void OnSaveRevisionClicked(FUnityVersionControlStateRef InSelectedFile);
	void OnRevertToRevisionClicked(TArray<FUnityVersionControlStateRef> InSelectedFiles);
	void OnShowHistoryClicked(TArray<FUnityVersionControlStateRef> InSelectedFiles);

	void SelectActors(const TArray<FAssetData> ActorsToSelect);
	void FocusActors(const TArray<FAssetData> ActorToFocus);
	void BrowseToAssets(const TArray<FAssetData> Assets);

	void StartRefreshStatus();
	void TickRefreshStatus(double InDeltaTime);
	void EndRefreshStatus();

	void RequestChangesetsRefresh();
	void RequestGetChangesetFiles(const FUnityVersionControlChangesetPtr& InSelectedChangeset);

	/** Source control callbacks */
	void OnGetChangesetsOperationComplete(const FSourceControlOperationRef& InOperation, ECommandResult::Type InResult);
	void OnGetChangesetFilesOperationComplete(const FSourceControlOperationRef& InOperation, ECommandResult::Type InResult);
	void OnSwitchToBranchOperationComplete(const FSourceControlOperationRef& InOperation, ECommandResult::Type InResult);
	void OnSwitchToChangesetOperationComplete(const FSourceControlOperationRef& InOperation, ECommandResult::Type InResult);
	void OnRevertToRevisionOperationComplete(const FSourceControlOperationRef& InOperation, ECommandResult::Type InResult);
	void OnSourceControlProviderChanged(ISourceControlProvider& OldProvider, ISourceControlProvider& NewProvider);

	/** Delegate handler for when source control state changes */
	void HandleSourceControlStateChanged();

	void OnSelectionChanged(FUnityVersionControlChangesetPtr InSelectedChangeset, ESelectInfo::Type SelectInfo);

	/** Double click to diff the selected changeset */
	void OnItemDoubleClicked(FUnityVersionControlChangesetRef InChangeset);

	/** Interpret F5 and Enter keys */
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;

private:
	TSharedPtr<SSearchBox> ChangesetsSearchBox;
	TSharedPtr<SSearchBox> FilesSearchBox;

	FName ChangesetsPrimarySortedColumn;
	FName ChangesetsSecondarySortedColumn;
	EColumnSortMode::Type ChangesetsPrimarySortMode = EColumnSortMode::Ascending;
	EColumnSortMode::Type ChangesetsSecondarySortMode = EColumnSortMode::None;

	FName FilesPrimarySortedColumn;
	FName FilesSecondarySortedColumn;
	EColumnSortMode::Type FilesPrimarySortMode = EColumnSortMode::Ascending;
	EColumnSortMode::Type FilesSecondarySortMode = EColumnSortMode::None;

	TArray<FName> ChangesetsHiddenColumnsList;

	bool bShouldRefresh = false;
	bool bSourceControlAvailable = false;

	FText RefreshStatus;
	bool bIsRefreshing = false;
	double RefreshStatusStartSecs;
	double LastRefreshTime = 0.0;

	int32 CurrentChangesetId;

	/** Ongoing notification for a long-running asynchronous source control operation, if any */
	FNotification Notification;

	TSharedPtr<SListView<FUnityVersionControlChangesetRef>> ChangesetsListView;
	TSharedPtr<TTextFilter<const FUnityVersionControlChangeset&>> ChangesetsSearchTextFilter;

	TMap<int32, FText> FromDateInDaysValues;
	int32 FromDateInDays = 30;

	TArray<FUnityVersionControlChangesetRef> SourceControlChangesets; // Full list from source (filtered by date)
	TArray<FUnityVersionControlChangesetRef> ChangesetRows; // Filtered list to display based on the search text filter

	TSharedPtr<SListView<FUnityVersionControlStateRef>> FilesListView;
	TSharedPtr<TTextFilter<const FUnityVersionControlState&>> FilesSearchTextFilter;

	FUnityVersionControlChangesetPtr SourceSelectedChangeset; // Current selected changeset from source control if any, with full list of files
	TArray<FUnityVersionControlStateRef> FileRows; // Filtered list to display based on the search text filter

	/** Delegate handle for the HandleSourceControlStateChanged function callback */
	FDelegateHandle SourceControlStateChangedDelegateHandle;
};
