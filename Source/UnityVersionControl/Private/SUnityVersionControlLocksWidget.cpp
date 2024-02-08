// Copyright (c) 2024 Unity Technologies

#include "SUnityVersionControlLocksWidget.h"

#include "UnityVersionControlLock.h"
#include "UnityVersionControlModule.h"
#include "UnityVersionControlOperations.h"
#include "UnityVersionControlProjectSettings.h"
#include "UnityVersionControlUtils.h"
#include "SUnityVersionControlLockRow.h"

#include "PackageUtils.h"

#include "ISourceControlModule.h"

#include "Logging/MessageLog.h"
#include "ToolMenus.h"
#include "ToolMenuContext.h"

#include "Runtime/Launch/Resources/Version.h"
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3
#include "Misc/ComparisonUtility.h"
#endif
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
#include "Styling/AppStyle.h"
#else
#include "EditorStyleSet.h"
#endif
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SWindow.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/SHeaderRow.h"

#define LOCTEXT_NAMESPACE "UnityVersionControlLockWindow"

void SUnityVersionControlLocksWidget::Construct(const FArguments& InArgs)
{
	ISourceControlModule::Get().RegisterProviderChanged(FSourceControlProviderChanged::FDelegate::CreateSP(this, &SUnityVersionControlLocksWidget::OnSourceControlProviderChanged));
	// register for any source control change to detect new local locks on check-out, and release of them on check-in
	SourceControlStateChangedDelegateHandle = ISourceControlModule::Get().GetProvider().RegisterSourceControlStateChanged_Handle(FSourceControlStateChanged::FDelegate::CreateSP(this, &SUnityVersionControlLocksWidget::HandleSourceControlStateChanged));

	CurrentBranchName = FUnityVersionControlModule::Get().GetProvider().GetBranchName();

	SearchTextFilter = MakeShared<TTextFilter<const FUnityVersionControlLock&>>(TTextFilter<const FUnityVersionControlLock&>::FItemToStringArray::CreateSP(this, &SUnityVersionControlLocksWidget::PopulateItemSearchStrings));
	SearchTextFilter->OnChanged().AddSP(this, &SUnityVersionControlLocksWidget::OnRefreshUI);

	ChildSlot
	[
		SNew(SVerticalBox)
		+SVerticalBox::Slot() // For the toolbar (Search box and Refresh button)
		.AutoHeight()
		[
			SNew(SBorder)
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
			.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
#else
			.BorderImage(FEditorStyle::GetBrush("DetailsView.CategoryBottom"))
#endif
			.Padding(4.0f)
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
				.HAlign(HAlign_Left)
				.VAlign(VAlign_Center)
				.AutoWidth()
				[
					CreateToolBar()
				]
				+SHorizontalBox::Slot()
				.MaxWidth(10.0f)
				[
					SNew(SSpacer)
				]
				+SHorizontalBox::Slot()
				.VAlign(VAlign_Center)
				.MaxWidth(300.0f)
				[
					SAssignNew(FileSearchBox, SSearchBox)
					.HintText(LOCTEXT("SearchLocks", "Search Locks"))
					.ToolTipText(LOCTEXT("PlasticLocksSearch_Tooltip", "Filter the list of locks by keyword."))
					.OnTextChanged(this, &SUnityVersionControlLocksWidget::OnSearchTextChanged)
				]
			]
		]
		+SVerticalBox::Slot() // The main content: the list of locks
		[
			CreateContentPanel()
		]
		+SVerticalBox::Slot() // Status bar (Always visible)
		.AutoHeight()
		[
			SNew(SBox)
			.Padding(FMargin(0.f, 3.f))
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
				.HAlign(HAlign_Left)
				.AutoWidth()
				[
					SNew(STextBlock)
					.Text_Lambda([this]() { return RefreshStatus; })
					.Margin(FMargin(5.f, 0.f))
				]
				+SHorizontalBox::Slot()
				.HAlign(HAlign_Right)
				[
					SNew(STextBlock)
					.Text_Lambda([this]() { return FText::FromString(CurrentBranchName); })
					.ToolTipText(LOCTEXT("PlasticBranchCurrent_Tooltip", "Current branch."))
				]
			]
		]
	];
}

TSharedRef<SWidget> SUnityVersionControlLocksWidget::CreateToolBar()
{
#if ENGINE_MAJOR_VERSION >= 5
	FSlimHorizontalToolBarBuilder ToolBarBuilder(nullptr, FMultiBoxCustomization::None);
#else
	FToolBarBuilder ToolBarBuilder(nullptr, FMultiBoxCustomization::None);
#endif

	ToolBarBuilder.AddToolBarButton(
		FUIAction(
			FExecuteAction::CreateLambda([this]() { RequestLocksRefresh(true); })),
		NAME_None,
		LOCTEXT("SourceControl_RefreshButton", "Refresh"),
		LOCTEXT("SourceControl_RefreshButton_Tooltip", "Refreshes locks from revision control provider."),
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "SourceControl.Actions.Refresh"));
#else
		FSlateIcon(FEditorStyle::GetStyleSetName(), "SourceControl.Actions.Refresh"));
#endif

	return ToolBarBuilder.MakeWidget();
}

TSharedRef<SWidget> SUnityVersionControlLocksWidget::CreateContentPanel()
{
	// Inspired by Engine\Source\Editor\SourceControlWindows\Private\SSourceControlChangelists.cpp
	// TSharedRef<SListView<FChangelistTreeItemPtr>> SSourceControlChangelistsWidget::CreateChangelistFilesView()

	UUnityVersionControlProjectSettings* Settings = GetMutableDefault<UUnityVersionControlProjectSettings>();
	if (!Settings->bShowLockIdColumn)
	{
		HiddenColumnsList.Add(UnityVersionControlLocksListViewColumn::ItemId::Id());
	}
	if (!Settings->bShowLockWorkspaceColumn)
	{
		HiddenColumnsList.Add(UnityVersionControlLocksListViewColumn::Workspace::Id());
	}
	if (!Settings->bShowLockDateColumn)
	{
		HiddenColumnsList.Add(UnityVersionControlLocksListViewColumn::Date::Id());
	}
	if (!Settings->bShowLockDestinationBranchColumn)
	{
		HiddenColumnsList.Add(UnityVersionControlLocksListViewColumn::DestinationBranch::Id());
	}

	TSharedRef<SListView<FUnityVersionControlLockRef>> LockView = SNew(SListView<FUnityVersionControlLockRef>)
		.ItemHeight(24.0f)
		.ListItemsSource(&LockRows)
		.OnGenerateRow(this, &SUnityVersionControlLocksWidget::OnGenerateRow)
		.SelectionMode(ESelectionMode::Multi)
		.OnContextMenuOpening(this, &SUnityVersionControlLocksWidget::OnOpenContextMenu)
		.OnItemToString_Debug_Lambda([this](FUnityVersionControlLockRef Lock) { return Lock->Path; })
		.HeaderRow
		(
			SNew(SHeaderRow)
			.CanSelectGeneratedColumn(true)
			.HiddenColumnsList(HiddenColumnsList)
			.OnHiddenColumnsListChanged(this, &SUnityVersionControlLocksWidget::OnHiddenColumnsListChanged)

			+SHeaderRow::Column(UnityVersionControlLocksListViewColumn::ItemId::Id())
			.DefaultLabel(UnityVersionControlLocksListViewColumn::ItemId::GetDisplayText())
			.DefaultTooltip(UnityVersionControlLocksListViewColumn::ItemId::GetToolTipText())
			.FillWidth(0.5f)
			.SortPriority(this, &SUnityVersionControlLocksWidget::GetColumnSortPriority, UnityVersionControlLocksListViewColumn::ItemId::Id())
			.SortMode(this, &SUnityVersionControlLocksWidget::GetColumnSortMode, UnityVersionControlLocksListViewColumn::ItemId::Id())
			.OnSort(this, &SUnityVersionControlLocksWidget::OnColumnSortModeChanged)

			+SHeaderRow::Column(UnityVersionControlLocksListViewColumn::Path::Id())
			.DefaultLabel(UnityVersionControlLocksListViewColumn::Path::GetDisplayText())
			.DefaultTooltip(UnityVersionControlLocksListViewColumn::Path::GetToolTipText())
			.ShouldGenerateWidget(true) // Ensure the column cannot be hidden (grayed out in the show/hide drop down menu)
			.FillWidth(4.0f)
			.SortPriority(this, &SUnityVersionControlLocksWidget::GetColumnSortPriority, UnityVersionControlLocksListViewColumn::Path::Id())
			.SortMode(this, &SUnityVersionControlLocksWidget::GetColumnSortMode, UnityVersionControlLocksListViewColumn::Path::Id())
			.OnSort(this, &SUnityVersionControlLocksWidget::OnColumnSortModeChanged)

			+SHeaderRow::Column(UnityVersionControlLocksListViewColumn::Status::Id())
			.DefaultLabel(UnityVersionControlLocksListViewColumn::Status::GetDisplayText())
			.DefaultTooltip(UnityVersionControlLocksListViewColumn::Status::GetToolTipText())
			.ShouldGenerateWidget(true) // Ensure the column cannot be hidden (grayed out in the show/hide drop down menu)
			.FillWidth(0.5f)
			.SortPriority(this, &SUnityVersionControlLocksWidget::GetColumnSortPriority, UnityVersionControlLocksListViewColumn::Status::Id())
			.SortMode(this, &SUnityVersionControlLocksWidget::GetColumnSortMode, UnityVersionControlLocksListViewColumn::Status::Id())
			.OnSort(this, &SUnityVersionControlLocksWidget::OnColumnSortModeChanged)

			+SHeaderRow::Column(UnityVersionControlLocksListViewColumn::Date::Id())
			.DefaultLabel(UnityVersionControlLocksListViewColumn::Date::GetDisplayText())
			.DefaultTooltip(UnityVersionControlLocksListViewColumn::Date::GetToolTipText())
			.FillWidth(1.5f)
			.SortPriority(this, &SUnityVersionControlLocksWidget::GetColumnSortPriority, UnityVersionControlLocksListViewColumn::Date::Id())
			.SortMode(this, &SUnityVersionControlLocksWidget::GetColumnSortMode, UnityVersionControlLocksListViewColumn::Date::Id())
			.OnSort(this, &SUnityVersionControlLocksWidget::OnColumnSortModeChanged)

			+SHeaderRow::Column(UnityVersionControlLocksListViewColumn::Owner::Id())
			.DefaultLabel(UnityVersionControlLocksListViewColumn::Owner::GetDisplayText())
			.DefaultTooltip(UnityVersionControlLocksListViewColumn::Owner::GetToolTipText())
			.ShouldGenerateWidget(true) // Ensure the column cannot be hidden (grayed out in the show/hide drop down menu)
			.FillWidth(2.0f)
			.SortPriority(this, &SUnityVersionControlLocksWidget::GetColumnSortPriority, UnityVersionControlLocksListViewColumn::Owner::Id())
			.SortMode(this, &SUnityVersionControlLocksWidget::GetColumnSortMode, UnityVersionControlLocksListViewColumn::Owner::Id())
			.OnSort(this, &SUnityVersionControlLocksWidget::OnColumnSortModeChanged)

			+SHeaderRow::Column(UnityVersionControlLocksListViewColumn::DestinationBranch::Id())
			.DefaultLabel(UnityVersionControlLocksListViewColumn::DestinationBranch::GetDisplayText())
			.DefaultTooltip(UnityVersionControlLocksListViewColumn::DestinationBranch::GetToolTipText())
			.FillWidth(2.0f)
			.SortPriority(this, &SUnityVersionControlLocksWidget::GetColumnSortPriority, UnityVersionControlLocksListViewColumn::DestinationBranch::Id())
			.SortMode(this, &SUnityVersionControlLocksWidget::GetColumnSortMode, UnityVersionControlLocksListViewColumn::DestinationBranch::Id())
			.OnSort(this, &SUnityVersionControlLocksWidget::OnColumnSortModeChanged)

			+SHeaderRow::Column(UnityVersionControlLocksListViewColumn::Branch::Id())
			.DefaultLabel(UnityVersionControlLocksListViewColumn::Branch::GetDisplayText())
			.DefaultTooltip(UnityVersionControlLocksListViewColumn::Branch::GetToolTipText())
			.ShouldGenerateWidget(true) // Ensure the column cannot be hidden (grayed out in the show/hide drop down menu)
			.FillWidth(2.0f)
			.SortPriority(this, &SUnityVersionControlLocksWidget::GetColumnSortPriority, UnityVersionControlLocksListViewColumn::Branch::Id())
			.SortMode(this, &SUnityVersionControlLocksWidget::GetColumnSortMode, UnityVersionControlLocksListViewColumn::Branch::Id())
			.OnSort(this, &SUnityVersionControlLocksWidget::OnColumnSortModeChanged)

			+SHeaderRow::Column(UnityVersionControlLocksListViewColumn::Workspace::Id())
			.DefaultLabel(UnityVersionControlLocksListViewColumn::Workspace::GetDisplayText())
			.DefaultTooltip(UnityVersionControlLocksListViewColumn::Workspace::GetToolTipText())
			.FillWidth(1.5f)
			.SortPriority(this, &SUnityVersionControlLocksWidget::GetColumnSortPriority, UnityVersionControlLocksListViewColumn::Workspace::Id())
			.SortMode(this, &SUnityVersionControlLocksWidget::GetColumnSortMode, UnityVersionControlLocksListViewColumn::Workspace::Id())
			.OnSort(this, &SUnityVersionControlLocksWidget::OnColumnSortModeChanged)
		);

	LocksListView = LockView;

	return LockView;
}

TSharedRef<ITableRow> SUnityVersionControlLocksWidget::OnGenerateRow(FUnityVersionControlLockRef InLock, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(SUnityVersionControlLockRow, OwnerTable)
		.LockToVisualize(InLock)
		.HighlightText_Lambda([this]() { return FileSearchBox->GetText(); });
}

void SUnityVersionControlLocksWidget::OnHiddenColumnsListChanged()
{
	// Update and save config to reload it on the next Editor sessions
	if (LocksListView && LocksListView->GetHeaderRow())
	{
		UUnityVersionControlProjectSettings* Settings = GetMutableDefault<UUnityVersionControlProjectSettings>();
		Settings->bShowLockIdColumn = true;
		Settings->bShowLockWorkspaceColumn = true;
		Settings->bShowLockDateColumn = true;
		Settings->bShowLockDestinationBranchColumn = true;

		for (const FName& ColumnId : LocksListView->GetHeaderRow()->GetHiddenColumnIds())
		{
			if (ColumnId == UnityVersionControlLocksListViewColumn::ItemId::Id())
			{
				Settings->bShowLockIdColumn = false;
			}
			else if (ColumnId == UnityVersionControlLocksListViewColumn::Workspace::Id())
			{
				Settings->bShowLockWorkspaceColumn = false;
			}
			else if (ColumnId == UnityVersionControlLocksListViewColumn::Date::Id())
			{
				Settings->bShowLockDateColumn = false;
			}
			else if (ColumnId == UnityVersionControlLocksListViewColumn::DestinationBranch::Id())
			{
				Settings->bShowLockDestinationBranchColumn = false;
			}
		}
		Settings->SaveConfig();
	}
}

void SUnityVersionControlLocksWidget::OnSearchTextChanged(const FText& InFilterText)
{
	SearchTextFilter->SetRawFilterText(InFilterText);
	FileSearchBox->SetError(SearchTextFilter->GetFilterErrorText());
}

void SUnityVersionControlLocksWidget::PopulateItemSearchStrings(const FUnityVersionControlLock& InItem, TArray<FString>& OutStrings)
{
	InItem.PopulateSearchString(OutStrings);
}

void SUnityVersionControlLocksWidget::OnRefreshUI()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(SUnityVersionControlLocksWidget::OnRefreshUI);

	const int32 ItemCount = SourceControlLocks.Num();
	LockRows.Empty(ItemCount);
	for (int32 ItemIndex = 0; ItemIndex < ItemCount; ++ItemIndex)
	{
		const FUnityVersionControlLockRef& Item = SourceControlLocks[ItemIndex];
		if (SearchTextFilter->PassesFilter(Item.Get()))
		{
			LockRows.Emplace(Item);
		}
	}

	if (GetListView())
	{
		SortLockView();
		GetListView()->RequestListRefresh();
	}
}

EColumnSortPriority::Type SUnityVersionControlLocksWidget::GetColumnSortPriority(const FName InColumnId) const
{
	if (InColumnId == PrimarySortedColumn)
	{
		return EColumnSortPriority::Primary;
	}
	else if (InColumnId == SecondarySortedColumn)
	{
		return EColumnSortPriority::Secondary;
	}

	return EColumnSortPriority::Max; // No specific priority.
}

EColumnSortMode::Type SUnityVersionControlLocksWidget::GetColumnSortMode(const FName InColumnId) const
{
	if (InColumnId == PrimarySortedColumn)
	{
		return PrimarySortMode;
	}
	else if (InColumnId == SecondarySortedColumn)
	{
		return SecondarySortMode;
	}

	return EColumnSortMode::None;
}

void SUnityVersionControlLocksWidget::OnColumnSortModeChanged(const EColumnSortPriority::Type InSortPriority, const FName& InColumnId, const EColumnSortMode::Type InSortMode)
{
	if (InSortPriority == EColumnSortPriority::Primary)
	{
		PrimarySortedColumn = InColumnId;
		PrimarySortMode = InSortMode;

		if (InColumnId == SecondarySortedColumn) // Cannot be primary and secondary at the same time.
		{
			SecondarySortedColumn = FName();
			SecondarySortMode = EColumnSortMode::None;
		}
	}
	else if (InSortPriority == EColumnSortPriority::Secondary)
	{
		SecondarySortedColumn = InColumnId;
		SecondarySortMode = InSortMode;
	}

	if (GetListView())
	{
		SortLockView();
		GetListView()->RequestListRefresh();
	}
}

void SUnityVersionControlLocksWidget::SortLockView()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(SUnityVersionControlLocksWidget::SortLockView);

	if (PrimarySortedColumn.IsNone() || LockRows.Num() == 0)
	{
		return; // No column selected for sorting or nothing to sort.
	}

	auto CompareItemIds = [](const FUnityVersionControlLock* Lhs, const FUnityVersionControlLock* Rhs)
	{
		return (Lhs->ItemId < Rhs->ItemId);
	};

	auto CompareStatuses = [](const FUnityVersionControlLock* Lhs, const FUnityVersionControlLock* Rhs)
	{
		return FCString::Stricmp(*Lhs->Status, *Rhs->Status);
	};

	auto ComparePaths = [](const FUnityVersionControlLock* Lhs, const FUnityVersionControlLock* Rhs)
	{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3
		return UE::ComparisonUtility::CompareNaturalOrder(*Lhs->Path, *Rhs->Path);
#else
		return FCString::Stricmp(*Lhs->Path, *Rhs->Path);
#endif
	};

	auto CompareOwners = [](const FUnityVersionControlLock* Lhs, const FUnityVersionControlLock* Rhs)
	{
		return FCString::Stricmp(*Lhs->Owner, *Rhs->Owner);
	};

	auto CompareDestinationBranches = [](const FUnityVersionControlLock* Lhs, const FUnityVersionControlLock* Rhs)
	{
		return FCString::Stricmp(*Lhs->DestinationBranch, *Rhs->DestinationBranch);
	};

	auto CompareBranches = [](const FUnityVersionControlLock* Lhs, const FUnityVersionControlLock* Rhs)
	{
		return FCString::Stricmp(*Lhs->Branch, *Rhs->Branch);
	};

	auto CompareWorkspaces = [](const FUnityVersionControlLock* Lhs, const FUnityVersionControlLock* Rhs)
	{
		return FCString::Stricmp(*Lhs->Workspace, *Rhs->Workspace);
	};

	auto CompareDates = [](const FUnityVersionControlLock* Lhs, const FUnityVersionControlLock* Rhs)
	{
		return Lhs->Date < Rhs->Date ? -1 : (Lhs->Date == Rhs->Date ? 0 : 1);
	};

	auto GetCompareFunc = [&](const FName& ColumnId)
	{
		if (ColumnId == UnityVersionControlLocksListViewColumn::ItemId::Id())
		{
			return TFunction<int32(const FUnityVersionControlLock*, const FUnityVersionControlLock*)>(CompareItemIds);
		}
		else if (ColumnId == UnityVersionControlLocksListViewColumn::Status::Id())
		{
			return TFunction<int32(const FUnityVersionControlLock*, const FUnityVersionControlLock*)>(CompareStatuses);
		}
		else if (ColumnId == UnityVersionControlLocksListViewColumn::Path::Id())
		{
			return TFunction<int32(const FUnityVersionControlLock*, const FUnityVersionControlLock*)>(ComparePaths);
		}
		else if (ColumnId == UnityVersionControlLocksListViewColumn::Owner::Id())
		{
			return TFunction<int32(const FUnityVersionControlLock*, const FUnityVersionControlLock*)>(CompareOwners);
		}
		else if (ColumnId == UnityVersionControlLocksListViewColumn::DestinationBranch::Id())
		{
			return TFunction<int32(const FUnityVersionControlLock*, const FUnityVersionControlLock*)>(CompareDestinationBranches);
		}
		else if (ColumnId == UnityVersionControlLocksListViewColumn::Branch::Id())
		{
			return TFunction<int32(const FUnityVersionControlLock*, const FUnityVersionControlLock*)>(CompareBranches);
		}
		else if (ColumnId == UnityVersionControlLocksListViewColumn::Workspace::Id())
		{
			return TFunction<int32(const FUnityVersionControlLock*, const FUnityVersionControlLock*)>(CompareWorkspaces);
		}
		else if (ColumnId == UnityVersionControlLocksListViewColumn::Date::Id())
		{
			return TFunction<int32(const FUnityVersionControlLock*, const FUnityVersionControlLock*)>(CompareDates);
		}
		else
		{
			checkNoEntry();
			return TFunction<int32(const FUnityVersionControlLock*, const FUnityVersionControlLock*)>();
		};
	};

	TFunction<int32(const FUnityVersionControlLock*, const FUnityVersionControlLock*)> PrimaryCompare = GetCompareFunc(PrimarySortedColumn);
	TFunction<int32(const FUnityVersionControlLock*, const FUnityVersionControlLock*)> SecondaryCompare;
	if (!SecondarySortedColumn.IsNone())
	{
		SecondaryCompare = GetCompareFunc(SecondarySortedColumn);
	}

	if (PrimarySortMode == EColumnSortMode::Ascending)
	{
		// NOTE: StableSort() would give a better experience when the sorted columns(s) has the same values and new values gets added, but it is slower
		//       with large changelists (7600 items was about 1.8x slower in average measured with Unreal Insight). Because this code runs in the main
		//       thread and can be invoked a lot, the trade off went if favor of speed.
		LockRows.Sort([this, &PrimaryCompare, &SecondaryCompare](const FUnityVersionControlLockPtr& Lhs, const FUnityVersionControlLockPtr& Rhs)
		{
			int32 Result = PrimaryCompare(static_cast<FUnityVersionControlLock*>(Lhs.Get()), static_cast<FUnityVersionControlLock*>(Rhs.Get()));
			if (Result < 0)
			{
				return true;
			}
			else if (Result > 0 || !SecondaryCompare)
			{
				return false;
			}
			else if (SecondarySortMode == EColumnSortMode::Ascending)
			{
				return SecondaryCompare(static_cast<FUnityVersionControlLock*>(Lhs.Get()), static_cast<FUnityVersionControlLock*>(Rhs.Get())) < 0;
			}
			else
			{
				return SecondaryCompare(static_cast<FUnityVersionControlLock*>(Lhs.Get()), static_cast<FUnityVersionControlLock*>(Rhs.Get())) > 0;
			}
		});
	}
	else
	{
		LockRows.Sort([this, &PrimaryCompare, &SecondaryCompare](const FUnityVersionControlLockPtr& Lhs, const FUnityVersionControlLockPtr& Rhs)
		{
			int32 Result = PrimaryCompare(static_cast<FUnityVersionControlLock*>(Lhs.Get()), static_cast<FUnityVersionControlLock*>(Rhs.Get()));
			if (Result > 0)
			{
				return true;
			}
			else if (Result < 0 || !SecondaryCompare)
			{
				return false;
			}
			else if (SecondarySortMode == EColumnSortMode::Ascending)
			{
				return SecondaryCompare(static_cast<FUnityVersionControlLock*>(Lhs.Get()), static_cast<FUnityVersionControlLock*>(Rhs.Get())) < 0;
			}
			else
			{
				return SecondaryCompare(static_cast<FUnityVersionControlLock*>(Lhs.Get()), static_cast<FUnityVersionControlLock*>(Rhs.Get())) > 0;
			}
		});
	}
}

TSharedPtr<SWidget> SUnityVersionControlLocksWidget::OnOpenContextMenu()
{
	const TArray<FUnityVersionControlLockRef> SelectedLocks = LocksListView->GetSelectedItems();
	if (SelectedLocks.Num() == 0)
	{
		return nullptr;
	}

	// Check to see if any of these locks are releasable, that is, if some of them are "Locked" instead of simply being "Retained"
	bool bCanReleaseLocks = false;
	for (const FUnityVersionControlLockRef& SelectedLock : SelectedLocks)
	{
		if (SelectedLock->bIsLocked)
		{
			bCanReleaseLocks = true;
			break;
		}
	}

	static const FText SelectASingleLockTooltip(LOCTEXT("SelectASingleLockTooltip", "Select a single lock."));
	static const FText UpdateUVCSTooltip(LOCTEXT("MergeLockXmlTooltip", "Update Unity Version Control (PlasticSCM) to 11.0.16.8101 with SmartLocks or later."));

	UToolMenus* ToolMenus = UToolMenus::Get();
	static const FName MenuName = "UnityVersionControl.LocksContextMenu";
	if (!ToolMenus->IsMenuRegistered(MenuName))
	{
		UToolMenu* RegisteredMenu = ToolMenus->RegisterMenu(MenuName);
		// Add section so it can be used as insert position for menu extensions
		RegisteredMenu->AddSection("Source Control");
	}

	// Build up the menu
	FToolMenuContext Context;
	UToolMenu* Menu = ToolMenus->GenerateMenu(MenuName, Context);

	FToolMenuSection& Section = *Menu->FindSection("Source Control");

	Section.AddMenuEntry(
		TEXT("ReleaseLock"),
		LOCTEXT("ReleaseLock", "Release"),
		LOCTEXT("ReleaseLocksTooltip", "Release Lock(s) on the selected assets.\nReleasing locks will allow other users to keep working on these files and retrieve locks (on the same branch, in the latest revision)."),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateSP(this, &SUnityVersionControlLocksWidget::OnReleaseLocksClicked, SelectedLocks),
			FCanExecuteAction::CreateLambda([bCanReleaseLocks]() { return bCanReleaseLocks; })
		)
	);
	Section.AddMenuEntry(
		TEXT("RemoveLock"),
		LOCTEXT("RemoveLock", "Remove"),
		LOCTEXT("RemoveLocksTooltip", "Remove Lock(s) on the selected assets.\nRemoving locks will allow other users to edit these files anywhere (on any branch) increasing the risk of future merge conflicts."),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateSP(this, &SUnityVersionControlLocksWidget::OnRemoveLocksClicked, SelectedLocks),
			FCanExecuteAction()
		)
	);

	return ToolMenus->GenerateWidget(Menu);
}

void SUnityVersionControlLocksWidget::OnReleaseLocksClicked(TArray<FUnityVersionControlLockRef> InSelectedLocks)
{
	ExecuteUnlock(MoveTemp(InSelectedLocks), false);
}

void SUnityVersionControlLocksWidget::OnRemoveLocksClicked(TArray<FUnityVersionControlLockRef> InSelectedLocks)
{
	ExecuteUnlock(MoveTemp(InSelectedLocks), true);
}

TArray<FString> LocksToFileNames(const TArray<FUnityVersionControlLockRef>& InSelectedLocks)
{
	TArray<FString> Files;

	// Note: remove the slash '/' from the end of the Workspace root to Combine it with server paths also starting with a slash
	const FString WorkspaceRoot = FUnityVersionControlModule::Get().GetProvider().GetPathToWorkspaceRoot().LeftChop(1);

	Files.Reserve(InSelectedLocks.Num());
	for (const FUnityVersionControlLockRef& SelectedLock : InSelectedLocks)
	{
		Files.AddUnique(FPaths::Combine(WorkspaceRoot, SelectedLock->Path));
	}

	return Files;
}

void SUnityVersionControlLocksWidget::ExecuteUnlock(TArray<FUnityVersionControlLockRef>&& InSelectedLocks, const bool bInRemove)
{
	const FText UnlockQuestion = FText::Format(bInRemove ?
		LOCTEXT("RemoveLocksDialog", "Removing locks will allow other users to edit these files anywhere (on any branch) increasing the risk of future merge conflicts. Would you like to remove {0} lock(s)?") :
		LOCTEXT("ReleaseLocksDialog", "Releasing locks will allow other users to keep working on these files and retrieve locks (on the same branch, in the latest revision). Would you like to release {0} lock(s)?"),
		FText::AsNumber(InSelectedLocks.Num()));
	const EAppReturnType::Type Choice = FMessageDialog::Open(
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3
		EAppMsgCategory::Info,
#endif
		EAppMsgType::YesNo, UnlockQuestion
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3
		, bInRemove ? LOCTEXT("RemoveLocksTitle", "Remove Lock(s)?") : LOCTEXT("ReleaseLocksTitle", "Release Lock(s)?")
#endif
	);
	if (Choice == EAppReturnType::Yes)
	{
		if (!Notification.IsInProgress())
		{
			// Launch a custom "Unlock" operation
			FUnityVersionControlProvider& Provider = FUnityVersionControlModule::Get().GetProvider();
			const TArray<FString> Files = LocksToFileNames(InSelectedLocks);
			TSharedRef<FPlasticUnlock, ESPMode::ThreadSafe> UnlockOperation = ISourceControlOperation::Create<FPlasticUnlock>();
			UnlockOperation->bRemove = bInRemove;
			UnlockOperation->Locks = MoveTemp(InSelectedLocks);
			const ECommandResult::Type Result = Provider.Execute(UnlockOperation, Files, EConcurrency::Asynchronous, FSourceControlOperationComplete::CreateSP(this, &SUnityVersionControlLocksWidget::OnUnlockOperationComplete));
			if (Result == ECommandResult::Succeeded)
			{
				// Display an ongoing notification during the whole operation (packages will be reloaded at the completion of the operation)
				Notification.DisplayInProgress(UnlockOperation->GetInProgressString());
				StartRefreshStatus();
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
}

void SUnityVersionControlLocksWidget::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	if (!ISourceControlModule::Get().IsEnabled() || (!FUnityVersionControlModule::Get().GetProvider().IsAvailable()))
	{
		return;
	}

	// Detect transitions of the source control being available/unavailable. Ex: When the user changes the source control in UI, the provider gets selected,
	// but it is not connected/available until the user accepts the settings. The source control doesn't have callback for availability and we want to refresh everything
	// once it gets available.
	if (ISourceControlModule::Get().IsEnabled() && !bSourceControlAvailable && ISourceControlModule::Get().GetProvider().IsAvailable())
	{
		bSourceControlAvailable = true;
		bShouldRefresh = true;
	}

	if (bShouldRefresh)
	{
		RequestLocksRefresh(false);
		bShouldRefresh = false;
	}

	if (bIsRefreshing)
	{
		TickRefreshStatus(InDeltaTime);
	}
}

void SUnityVersionControlLocksWidget::StartRefreshStatus()
{
	if (!bIsRefreshing)
	{
		bIsRefreshing = true;
		RefreshStatusStartSecs = FPlatformTime::Seconds();
	}
}

void SUnityVersionControlLocksWidget::TickRefreshStatus(double InDeltaTime)
{
	const int32 RefreshStatusTimeElapsed = static_cast<int32>(FPlatformTime::Seconds() - RefreshStatusStartSecs);
	RefreshStatus = FText::Format(LOCTEXT("UnityVersionControl_RefreshLocks", "Refreshing locks... ({0} s)"), FText::AsNumber(RefreshStatusTimeElapsed));
}

void SUnityVersionControlLocksWidget::EndRefreshStatus()
{
	bIsRefreshing = false;
	RefreshStatus = FText::GetEmpty();
}

void SUnityVersionControlLocksWidget::RequestLocksRefresh(const bool bInInvalidateLocksCache)
{
	if (!ISourceControlModule::Get().IsEnabled() || (!FUnityVersionControlModule::Get().GetProvider().IsAvailable()))
	{
		return;
	}

	StartRefreshStatus();

	if (bInInvalidateLocksCache)
	{
		UnityVersionControlUtils::InvalidateLocksCache();
	}

	TSharedRef<FPlasticGetLocks, ESPMode::ThreadSafe> GetLocksOperation = ISourceControlOperation::Create<FPlasticGetLocks>();

	FUnityVersionControlProvider& Provider = FUnityVersionControlModule::Get().GetProvider();
	Provider.Execute(GetLocksOperation, EConcurrency::Asynchronous, FSourceControlOperationComplete::CreateSP(this, &SUnityVersionControlLocksWidget::OnGetLocksOperationComplete));
}

void SUnityVersionControlLocksWidget::OnGetLocksOperationComplete(const FSourceControlOperationRef& InOperation, ECommandResult::Type InResult)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(SUnityVersionControlLocksWidget::OnGetLocksOperationComplete);

	TSharedRef<FPlasticGetLocks, ESPMode::ThreadSafe> OperationGetLocks = StaticCastSharedRef<FPlasticGetLocks>(InOperation);
	SourceControlLocks = MoveTemp(OperationGetLocks->Locks);

	CurrentBranchName = FUnityVersionControlModule::Get().GetProvider().GetBranchName();

	EndRefreshStatus();
	OnRefreshUI();
}

void SUnityVersionControlLocksWidget::OnUnlockOperationComplete(const FSourceControlOperationRef& InOperation, ECommandResult::Type InResult)
{
	// Ask for a full refresh of the list of locks (and don't call EndRefreshStatus() yet)
	bShouldRefresh = true;

	Notification.RemoveInProgress();

	FNotification::DisplayResult(InOperation, InResult);
}

void SUnityVersionControlLocksWidget::OnSourceControlProviderChanged(ISourceControlProvider& OldProvider, ISourceControlProvider& NewProvider)
{
	bSourceControlAvailable = NewProvider.IsAvailable(); // Check if it is connected.
	bShouldRefresh = true;

	if (&NewProvider != &OldProvider)
	{
		LockRows.Reset();
		if (GetListView())
		{
			GetListView()->RequestListRefresh();
		}
	}
}

void SUnityVersionControlLocksWidget::HandleSourceControlStateChanged()
{
	bShouldRefresh = true;
	if (GetListView())
	{
		GetListView()->RequestListRefresh();
	}
}

FReply SUnityVersionControlLocksWidget::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::F5)
	{
		// Pressing F5 refreshes the list of locks
		RequestLocksRefresh(true);
		return FReply::Handled();
	}
	else if ((InKeyEvent.GetKey() == EKeys::Delete) || (InKeyEvent.GetKey() == EKeys::BackSpace))
	{
		// Pressing Delete or BackSpace removes the selected locks
		const TArray<FUnityVersionControlLockRef> SelectedLocks = LocksListView->GetSelectedItems();
		if (SelectedLocks.Num() > 0)
		{
			OnRemoveLocksClicked(SelectedLocks);
		}
		return FReply::Handled();
	}

	return FReply::Unhandled();
}

#undef LOCTEXT_NAMESPACE
