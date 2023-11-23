// Copyright (c) 2023 Unity Technologies

#pragma once

#include "CoreMinimal.h"

#include "Notification.h"

#include "Misc/TextFilter.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/SListView.h"

#include "ISourceControlOperation.h"
#include "ISourceControlProvider.h"

typedef TSharedRef<class FUnityVersionControlBranch, ESPMode::ThreadSafe> FUnityVersionControlBranchRef;

class SSearchBox;

// Widget displaying the list of branches in the tab window, see FUnityVersionControlBranchesWindow
class SUnityVersionControlBranchesWidget : public SCompoundWidget
{
	SLATE_BEGIN_ARGS(SUnityVersionControlBranchesWidget) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

private:
	TSharedRef<SWidget> CreateToolBar();
	TSharedRef<SWidget> CreateContentPanel();

	TSharedRef<ITableRow> OnGenerateRow(FUnityVersionControlBranchRef InBranch, const TSharedRef<STableViewBase>& OwnerTable);
	void OnHiddenColumnsListChanged();

	void OnSearchTextChanged(const FText& InFilterText);
	void PopulateItemSearchStrings(const FUnityVersionControlBranch& InItem, TArray<FString>& OutStrings);

	TSharedRef<SWidget> BuildFromDateDropDownMenu();
	void OnFromDateChanged(int32 InFromDateInDays);

	void OnRefreshUI();

	EColumnSortPriority::Type GetColumnSortPriority(const FName InColumnId) const;
	EColumnSortMode::Type GetColumnSortMode(const FName InColumnId) const;
	void OnColumnSortModeChanged(const EColumnSortPriority::Type InSortPriority, const FName& InColumnId, const EColumnSortMode::Type InSortMode);

	void SortBranchView();
	FString GetSelectedBranch();

	TSharedPtr<SWidget> OnOpenContextMenu();
	void OnSwitchToBranchClicked(FString InBranchName);

	void StartRefreshStatus();
	void TickRefreshStatus(double InDeltaTime);
	void EndRefreshStatus();

	void RequestBranchesRefresh();

	/** Source control callbacks */
	void OnGetBranchesOperationComplete(const FSourceControlOperationRef& InOperation, ECommandResult::Type InResult);
	void OnSwitchToBranchOperationComplete(const FSourceControlOperationRef& InOperation, ECommandResult::Type InResult);
	void OnSourceControlProviderChanged(ISourceControlProvider& OldProvider, ISourceControlProvider& NewProvider);

	SListView<FUnityVersionControlBranchRef>* GetListView() const
	{
		return BranchesListView.Get();
	}

private:
	TSharedPtr<SSearchBox> FileSearchBox;

	FName PrimarySortedColumn;
	FName SecondarySortedColumn;
	EColumnSortMode::Type PrimarySortMode = EColumnSortMode::Ascending;
	EColumnSortMode::Type SecondarySortMode = EColumnSortMode::None;

	TArray<FName> HiddenColumnsList;

	bool bShouldRefresh = false;
	bool bSourceControlAvailable = false;

	FText RefreshStatus;
	bool bIsRefreshing = false;
	double RefreshStatusStartSecs;

	FString CurrentBranchName;

	/** Ongoing notification for a long-running asynchronous source control operation, if any */
	FNotification Notification;

	TSharedPtr<SListView<FUnityVersionControlBranchRef>> BranchesListView;
	TSharedPtr<TTextFilter<const FUnityVersionControlBranch&>> SearchTextFilter;

	TMap<int32, FText> FromDateInDaysValues;
	int32 FromDateInDays = 30;

	TArray<FUnityVersionControlBranchRef> SourceControlBranches; // Full list from source (filtered by date)
	TArray<FUnityVersionControlBranchRef> BranchRows; // Filtered list to display based on the search text filter
};
