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

typedef TSharedRef<class FUnityVersionControlLock, ESPMode::ThreadSafe> FUnityVersionControlLockRef;

class SSearchBox;
class SWindow;

// Widget displaying the list of locks in the tab window, see FUnityVersionControlLocksWindow
class SUnityVersionControlLocksWidget : public SCompoundWidget
{
	SLATE_BEGIN_ARGS(SUnityVersionControlLocksWidget) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

private:
	TSharedRef<SWidget> CreateToolBar();
	TSharedRef<SWidget> CreateContentPanel();

	TSharedRef<ITableRow> OnGenerateRow(FUnityVersionControlLockRef InLock, const TSharedRef<STableViewBase>& OwnerTable);
	void OnHiddenColumnsListChanged();

	void OnSearchTextChanged(const FText& InFilterText);
	void PopulateItemSearchStrings(const FUnityVersionControlLock& InItem, TArray<FString>& OutStrings);

	void OnRefreshUI();

	EColumnSortPriority::Type GetColumnSortPriority(const FName InColumnId) const;
	EColumnSortMode::Type GetColumnSortMode(const FName InColumnId) const;
	void OnColumnSortModeChanged(const EColumnSortPriority::Type InSortPriority, const FName& InColumnId, const EColumnSortMode::Type InSortMode);

	void SortLockView();

	TSharedPtr<SWidget> OnOpenContextMenu();

	void OnReleaseLocksClicked(TArray<FUnityVersionControlLockRef> InSelectedLocks);
	void OnRemoveLocksClicked(TArray<FUnityVersionControlLockRef> InSelectedLocks);
	void ExecuteUnlock(TArray<FUnityVersionControlLockRef>&& InSelectedLocks, const bool bInRemove);

	void StartRefreshStatus();
	void TickRefreshStatus(double InDeltaTime);
	void EndRefreshStatus();

	void RequestLocksRefresh(const bool bInInvalidateLocksCache);

	/** Source control callbacks */
	void OnGetLocksOperationComplete(const FSourceControlOperationRef& InOperation, ECommandResult::Type InResult);
	void OnUnlockOperationComplete(const FSourceControlOperationRef& InOperation, ECommandResult::Type InResult);
	void OnSourceControlProviderChanged(ISourceControlProvider& OldProvider, ISourceControlProvider& NewProvider);

	/** Delegate handler for when source control state changes */
	void HandleSourceControlStateChanged();

	SListView<FUnityVersionControlLockRef>* GetListView() const
	{
		return LocksListView.Get();
	}

	/** Interpret F5, Enter and Delete keys */
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;

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

	TSharedPtr<SListView<FUnityVersionControlLockRef>> LocksListView;
	TSharedPtr<TTextFilter<const FUnityVersionControlLock&>> SearchTextFilter;

	TArray<FUnityVersionControlLockRef> SourceControlLocks; // Full list from source (filtered by date)
	TArray<FUnityVersionControlLockRef> LockRows; // Filtered list to display based on the search text filter

	/** Delegate handle for the HandleSourceControlStateChanged function callback */
	FDelegateHandle SourceControlStateChangedDelegateHandle;
};
