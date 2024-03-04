// Copyright (c) 2024 Unity Technologies

#include "SUnityVersionControlBranchesWidget.h"

#include "UnityVersionControlModule.h"
#include "UnityVersionControlOperations.h"
#include "UnityVersionControlProjectSettings.h"
#include "UnityVersionControlBranch.h"
#include "UnityVersionControlVersions.h"
#include "SUnityVersionControlBranchRow.h"
#include "SUnityVersionControlCreateBranch.h"
#include "SUnityVersionControlDeleteBranches.h"
#include "SUnityVersionControlRenameBranch.h"

#include "PackageUtils.h"

#include "ISourceControlModule.h"

#include "Logging/MessageLog.h"
#include "ToolMenus.h"
#include "ToolMenuContext.h"

#include "Runtime/Launch/Resources/Version.h"
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3
#include "Misc/ComparisonUtility.h"
#endif
#include "Misc/MessageDialog.h"
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
#include "Styling/AppStyle.h"
#else
#include "EditorStyleSet.h"
#endif
#include "Framework/Application/SlateApplication.h"
#include "Framework/Docking/TabManager.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/SHeaderRow.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SWindow.h"

#define LOCTEXT_NAMESPACE "UnityVersionControlBranchesWindow"

void SUnityVersionControlBranchesWidget::Construct(const FArguments& InArgs)
{
	ISourceControlModule::Get().RegisterProviderChanged(FSourceControlProviderChanged::FDelegate::CreateSP(this, &SUnityVersionControlBranchesWidget::OnSourceControlProviderChanged));

	CurrentBranchName = FUnityVersionControlModule::Get().GetProvider().GetBranchName();

	SearchTextFilter = MakeShared<TTextFilter<const FUnityVersionControlBranch&>>(TTextFilter<const FUnityVersionControlBranch&>::FItemToStringArray::CreateSP(this, &SUnityVersionControlBranchesWidget::PopulateItemSearchStrings));
	SearchTextFilter->OnChanged().AddSP(this, &SUnityVersionControlBranchesWidget::OnRefreshUI);

	FromDateInDaysValues.Add(TPair<int32, FText>(7, FText::FromString(TEXT("Last week"))));
	FromDateInDaysValues.Add(TPair<int32, FText>(30, FText::FromString(TEXT("Last month"))));
	FromDateInDaysValues.Add(TPair<int32, FText>(90, FText::FromString(TEXT("Last 3 months"))));
	FromDateInDaysValues.Add(TPair<int32, FText>(365, FText::FromString(TEXT("Last year"))));
	FromDateInDaysValues.Add(TPair<int32, FText>(-1, FText::FromString(TEXT("All time"))));

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
					.HintText(LOCTEXT("SearchBranches", "Search Branches"))
					.ToolTipText(LOCTEXT("PlasticBranchesSearch_Tooltip", "Filter the list of branches by keyword."))
					.OnTextChanged(this, &SUnityVersionControlBranchesWidget::OnSearchTextChanged)
				]
				+SHorizontalBox::Slot()
				.VAlign(VAlign_Center)
				.MaxWidth(125.0f)
				.Padding(10.f, 0.f)
				[
					SNew(SComboButton)
					.ToolTipText(LOCTEXT("PlasticBranchesDate_Tooltip", "Filter the list of branches by date of creation."))
					.OnGetMenuContent(this, &SUnityVersionControlBranchesWidget::BuildFromDateDropDownMenu)
					.ButtonContent()
					[
						SNew(STextBlock)
						.Text_Lambda([this]() { return FromDateInDaysValues[FromDateInDays]; })
					]
				]
			]
		]
		+SVerticalBox::Slot() // The main content: the list of branches
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

TSharedRef<SWidget> SUnityVersionControlBranchesWidget::CreateToolBar()
{
#if ENGINE_MAJOR_VERSION >= 5
	FSlimHorizontalToolBarBuilder ToolBarBuilder(nullptr, FMultiBoxCustomization::None);
#else
	FToolBarBuilder ToolBarBuilder(nullptr, FMultiBoxCustomization::None);
#endif

	ToolBarBuilder.AddToolBarButton(
		FUIAction(
			FExecuteAction::CreateLambda([this]() { RequestBranchesRefresh(); })),
		NAME_None,
		LOCTEXT("SourceControl_RefreshButton", "Refresh"),
		LOCTEXT("SourceControl_RefreshButton_Tooltip", "Refreshes branches from revision control provider."),
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "SourceControl.Actions.Refresh"));
#else
		FSlateIcon(FEditorStyle::GetStyleSetName(), "SourceControl.Actions.Refresh"));
#endif

	return ToolBarBuilder.MakeWidget();
}

TSharedRef<SWidget> SUnityVersionControlBranchesWidget::CreateContentPanel()
{
	// Inspired by Engine\Source\Editor\SourceControlWindows\Private\SSourceControlChangelists.cpp
	// TSharedRef<SListView<FChangelistTreeItemPtr>> SSourceControlChangelistsWidget::CreateChangelistFilesView()

	UUnityVersionControlProjectSettings* Settings = GetMutableDefault<UUnityVersionControlProjectSettings>();
	if (!Settings->bShowBranchRepositoryColumn)
	{
		HiddenColumnsList.Add(UnityVersionControlBranchesListViewColumn::Repository::Id());
	}
	if (!Settings->bShowBranchCreatedByColumn)
	{
		HiddenColumnsList.Add(UnityVersionControlBranchesListViewColumn::CreatedBy::Id());
	}
	if (!Settings->bShowBranchDateColumn)
	{
		HiddenColumnsList.Add(UnityVersionControlBranchesListViewColumn::Date::Id());
	}
	if (!Settings->bShowBranchCommentColumn)
	{
		HiddenColumnsList.Add(UnityVersionControlBranchesListViewColumn::Comment::Id());
	}

	TSharedRef<SListView<FUnityVersionControlBranchRef>> BranchView = SNew(SListView<FUnityVersionControlBranchRef>)
		.ItemHeight(24.0f)
		.ListItemsSource(&BranchRows)
		.OnGenerateRow(this, &SUnityVersionControlBranchesWidget::OnGenerateRow)
		.SelectionMode(ESelectionMode::Multi)
		.OnContextMenuOpening(this, &SUnityVersionControlBranchesWidget::OnOpenContextMenu)
		.OnItemToString_Debug_Lambda([this](FUnityVersionControlBranchRef Branch) { return Branch->Name; })
		.HeaderRow
		(
			SNew(SHeaderRow)
			.CanSelectGeneratedColumn(true)
			.HiddenColumnsList(HiddenColumnsList)
			.OnHiddenColumnsListChanged(this, &SUnityVersionControlBranchesWidget::OnHiddenColumnsListChanged)

			+SHeaderRow::Column(UnityVersionControlBranchesListViewColumn::Name::Id())
			.DefaultLabel(UnityVersionControlBranchesListViewColumn::Name::GetDisplayText())
			.DefaultTooltip(UnityVersionControlBranchesListViewColumn::Name::GetToolTipText())
			.ShouldGenerateWidget(true) // Ensure the column cannot be hidden (grayed out in the show/hide drop down menu)
			.FillWidth(2.0f)
			.SortPriority(this, &SUnityVersionControlBranchesWidget::GetColumnSortPriority, UnityVersionControlBranchesListViewColumn::Name::Id())
			.SortMode(this, &SUnityVersionControlBranchesWidget::GetColumnSortMode, UnityVersionControlBranchesListViewColumn::Name::Id())
			.OnSort(this, &SUnityVersionControlBranchesWidget::OnColumnSortModeChanged)

			+SHeaderRow::Column(UnityVersionControlBranchesListViewColumn::Repository::Id())
			.DefaultLabel(UnityVersionControlBranchesListViewColumn::Repository::GetDisplayText())
			.DefaultTooltip(UnityVersionControlBranchesListViewColumn::Repository::GetToolTipText())
			.FillWidth(1.5f)
			.SortPriority(this, &SUnityVersionControlBranchesWidget::GetColumnSortPriority, UnityVersionControlBranchesListViewColumn::Repository::Id())
			.SortMode(this, &SUnityVersionControlBranchesWidget::GetColumnSortMode, UnityVersionControlBranchesListViewColumn::Repository::Id())
			.OnSort(this, &SUnityVersionControlBranchesWidget::OnColumnSortModeChanged)

			+SHeaderRow::Column(UnityVersionControlBranchesListViewColumn::CreatedBy::Id())
			.DefaultLabel(UnityVersionControlBranchesListViewColumn::CreatedBy::GetDisplayText())
			.DefaultTooltip(UnityVersionControlBranchesListViewColumn::CreatedBy::GetToolTipText())
			.FillWidth(2.5f)
			.SortPriority(this, &SUnityVersionControlBranchesWidget::GetColumnSortPriority, UnityVersionControlBranchesListViewColumn::CreatedBy::Id())
			.SortMode(this, &SUnityVersionControlBranchesWidget::GetColumnSortMode, UnityVersionControlBranchesListViewColumn::CreatedBy::Id())
			.OnSort(this, &SUnityVersionControlBranchesWidget::OnColumnSortModeChanged)

			+SHeaderRow::Column(UnityVersionControlBranchesListViewColumn::Date::Id())
			.DefaultLabel(UnityVersionControlBranchesListViewColumn::Date::GetDisplayText())
			.DefaultTooltip(UnityVersionControlBranchesListViewColumn::Date::GetToolTipText())
			.FillWidth(1.5f)
			.SortPriority(this, &SUnityVersionControlBranchesWidget::GetColumnSortPriority, UnityVersionControlBranchesListViewColumn::Date::Id())
			.SortMode(this, &SUnityVersionControlBranchesWidget::GetColumnSortMode, UnityVersionControlBranchesListViewColumn::Date::Id())
			.OnSort(this, &SUnityVersionControlBranchesWidget::OnColumnSortModeChanged)

			+SHeaderRow::Column(UnityVersionControlBranchesListViewColumn::Comment::Id())
			.DefaultLabel(UnityVersionControlBranchesListViewColumn::Comment::GetDisplayText())
			.DefaultTooltip(UnityVersionControlBranchesListViewColumn::Comment::GetToolTipText())
			.FillWidth(5.0f)
			.SortPriority(this, &SUnityVersionControlBranchesWidget::GetColumnSortPriority, UnityVersionControlBranchesListViewColumn::Comment::Id())
			.SortMode(this, &SUnityVersionControlBranchesWidget::GetColumnSortMode, UnityVersionControlBranchesListViewColumn::Comment::Id())
			.OnSort(this, &SUnityVersionControlBranchesWidget::OnColumnSortModeChanged)
		);

	BranchesListView = BranchView;

	return BranchView;
}

TSharedRef<ITableRow> SUnityVersionControlBranchesWidget::OnGenerateRow(FUnityVersionControlBranchRef InBranch, const TSharedRef<STableViewBase>& OwnerTable)
{
	const bool bIsCurrentBranch = InBranch->Name == CurrentBranchName;
	return SNew(SUnityVersionControlBranchRow, OwnerTable)
		.BranchToVisualize(InBranch)
		.bIsCurrentBranch(bIsCurrentBranch)
		.HighlightText_Lambda([this]() { return FileSearchBox->GetText(); });
}

void SUnityVersionControlBranchesWidget::OnHiddenColumnsListChanged()
{
	// Update and save config to reload it on the next Editor sessions
	if (BranchesListView && BranchesListView->GetHeaderRow())
	{
		UUnityVersionControlProjectSettings* Settings = GetMutableDefault<UUnityVersionControlProjectSettings>();
		Settings->bShowBranchRepositoryColumn = true;
		Settings->bShowBranchCreatedByColumn = true;
		Settings->bShowBranchDateColumn = true;
		Settings->bShowBranchCommentColumn = true;

		for (const FName& ColumnId : BranchesListView->GetHeaderRow()->GetHiddenColumnIds())
		{
			if (ColumnId == UnityVersionControlBranchesListViewColumn::Repository::Id())
			{
				Settings->bShowBranchRepositoryColumn = false;
			}
			else if (ColumnId == UnityVersionControlBranchesListViewColumn::CreatedBy::Id())
			{
				Settings->bShowBranchCreatedByColumn = false;
			}
			else if (ColumnId == UnityVersionControlBranchesListViewColumn::Date::Id())
			{
				Settings->bShowBranchDateColumn = false;
			}
			else if (ColumnId == UnityVersionControlBranchesListViewColumn::Comment::Id())
			{
				Settings->bShowBranchCommentColumn = false;
			}
		}
		Settings->SaveConfig();
	}
}

void SUnityVersionControlBranchesWidget::OnSearchTextChanged(const FText& InFilterText)
{
	SearchTextFilter->SetRawFilterText(InFilterText);
	FileSearchBox->SetError(SearchTextFilter->GetFilterErrorText());
}

void SUnityVersionControlBranchesWidget::PopulateItemSearchStrings(const FUnityVersionControlBranch& InItem, TArray<FString>& OutStrings)
{
	InItem.PopulateSearchString(OutStrings);
}

void SUnityVersionControlBranchesWidget::OnFromDateChanged(int32 InFromDateInDays)
{
	FromDateInDays = InFromDateInDays;

	RequestBranchesRefresh();
}

TSharedRef<SWidget> SUnityVersionControlBranchesWidget::BuildFromDateDropDownMenu()
{
	FMenuBuilder MenuBuilder(true, NULL);

	for (const auto & FromDateInDaysValue : FromDateInDaysValues)
	{
		FUIAction MenuAction(FExecuteAction::CreateSP(this, &SUnityVersionControlBranchesWidget::OnFromDateChanged, FromDateInDaysValue.Key));
		MenuBuilder.AddMenuEntry(FromDateInDaysValue.Value, FromDateInDaysValue.Value, FSlateIcon(), MenuAction);
	}

	return MenuBuilder.MakeWidget();
}

void SUnityVersionControlBranchesWidget::OnRefreshUI()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(SUnityVersionControlBranchesWidget::OnRefreshUI);

	const int32 ItemCount = SourceControlBranches.Num();
	BranchRows.Empty(ItemCount);
	for (int32 ItemIndex = 0; ItemIndex < ItemCount; ++ItemIndex)
	{
		const FUnityVersionControlBranchRef& Item = SourceControlBranches[ItemIndex];
		if (SearchTextFilter->PassesFilter(Item.Get()))
		{
			BranchRows.Emplace(Item);
		}
	}

	if (GetListView())
	{
		SortBranchView();
		GetListView()->RequestListRefresh();
	}
}

EColumnSortPriority::Type SUnityVersionControlBranchesWidget::GetColumnSortPriority(const FName InColumnId) const
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

EColumnSortMode::Type SUnityVersionControlBranchesWidget::GetColumnSortMode(const FName InColumnId) const
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

void SUnityVersionControlBranchesWidget::OnColumnSortModeChanged(const EColumnSortPriority::Type InSortPriority, const FName& InColumnId, const EColumnSortMode::Type InSortMode)
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
		SortBranchView();
		GetListView()->RequestListRefresh();
	}
}

void SUnityVersionControlBranchesWidget::SortBranchView()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(SUnityVersionControlBranchesWidget::SortBranchView);

	if (PrimarySortedColumn.IsNone() || BranchRows.Num() == 0)
	{
		return; // No column selected for sorting or nothing to sort.
	}

	auto CompareNames = [](const FUnityVersionControlBranch* Lhs, const FUnityVersionControlBranch* Rhs)
	{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3
		return UE::ComparisonUtility::CompareNaturalOrder(*Lhs->Name, *Rhs->Name);
#else
		return FCString::Stricmp(*Lhs->Name, *Rhs->Name);
#endif
	};

	auto CompareRepository = [](const FUnityVersionControlBranch* Lhs, const FUnityVersionControlBranch* Rhs)
	{
		return FCString::Stricmp(*Lhs->Repository, *Rhs->Repository);
	};

	auto CompareCreatedBy = [](const FUnityVersionControlBranch* Lhs, const FUnityVersionControlBranch* Rhs)
	{
		return FCString::Stricmp(*Lhs->CreatedBy, *Rhs->CreatedBy);
	};

	auto CompareDate = [](const FUnityVersionControlBranch* Lhs, const FUnityVersionControlBranch* Rhs)
	{
		return Lhs->Date < Rhs->Date ? -1 : (Lhs->Date == Rhs->Date ? 0 : 1);
	};

	auto CompareComment = [](const FUnityVersionControlBranch* Lhs, const FUnityVersionControlBranch* Rhs)
	{
		return FCString::Stricmp(*Lhs->Comment, *Rhs->Comment);
	};

	auto GetCompareFunc = [&](const FName& ColumnId)
	{
		if (ColumnId == UnityVersionControlBranchesListViewColumn::Name::Id())
		{
			return TFunction<int32(const FUnityVersionControlBranch*, const FUnityVersionControlBranch*)>(CompareNames);
		}
		else if (ColumnId == UnityVersionControlBranchesListViewColumn::Repository::Id())
		{
			return TFunction<int32(const FUnityVersionControlBranch*, const FUnityVersionControlBranch*)>(CompareRepository);
		}
		else if (ColumnId == UnityVersionControlBranchesListViewColumn::CreatedBy::Id())
		{
			return TFunction<int32(const FUnityVersionControlBranch*, const FUnityVersionControlBranch*)>(CompareCreatedBy);
		}
		else if (ColumnId == UnityVersionControlBranchesListViewColumn::Date::Id())
		{
			return TFunction<int32(const FUnityVersionControlBranch*, const FUnityVersionControlBranch*)>(CompareDate);
		}
		else if (ColumnId == UnityVersionControlBranchesListViewColumn::Comment::Id())
		{
			return TFunction<int32(const FUnityVersionControlBranch*, const FUnityVersionControlBranch*)>(CompareComment);
		}
		else
		{
			checkNoEntry();
			return TFunction<int32(const FUnityVersionControlBranch*, const FUnityVersionControlBranch*)>();
		};
	};

	TFunction<int32(const FUnityVersionControlBranch*, const FUnityVersionControlBranch*)> PrimaryCompare = GetCompareFunc(PrimarySortedColumn);
	TFunction<int32(const FUnityVersionControlBranch*, const FUnityVersionControlBranch*)> SecondaryCompare;
	if (!SecondarySortedColumn.IsNone())
	{
		SecondaryCompare = GetCompareFunc(SecondarySortedColumn);
	}

	if (PrimarySortMode == EColumnSortMode::Ascending)
	{
		// NOTE: StableSort() would give a better experience when the sorted columns(s) has the same values and new values gets added, but it is slower
		//       with large changelists (7600 items was about 1.8x slower in average measured with Unreal Insight). Because this code runs in the main
		//       thread and can be invoked a lot, the trade off went if favor of speed.
		BranchRows.Sort([this, &PrimaryCompare, &SecondaryCompare](const FUnityVersionControlBranchPtr& Lhs, const FUnityVersionControlBranchPtr& Rhs)
		{
			int32 Result = PrimaryCompare(static_cast<FUnityVersionControlBranch*>(Lhs.Get()), static_cast<FUnityVersionControlBranch*>(Rhs.Get()));
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
				return SecondaryCompare(static_cast<FUnityVersionControlBranch*>(Lhs.Get()), static_cast<FUnityVersionControlBranch*>(Rhs.Get())) < 0;
			}
			else
			{
				return SecondaryCompare(static_cast<FUnityVersionControlBranch*>(Lhs.Get()), static_cast<FUnityVersionControlBranch*>(Rhs.Get())) > 0;
			}
		});
	}
	else
	{
		BranchRows.Sort([this, &PrimaryCompare, &SecondaryCompare](const FUnityVersionControlBranchPtr& Lhs, const FUnityVersionControlBranchPtr& Rhs)
		{
			int32 Result = PrimaryCompare(static_cast<FUnityVersionControlBranch*>(Lhs.Get()), static_cast<FUnityVersionControlBranch*>(Rhs.Get()));
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
				return SecondaryCompare(static_cast<FUnityVersionControlBranch*>(Lhs.Get()), static_cast<FUnityVersionControlBranch*>(Rhs.Get())) < 0;
			}
			else
			{
				return SecondaryCompare(static_cast<FUnityVersionControlBranch*>(Lhs.Get()), static_cast<FUnityVersionControlBranch*>(Rhs.Get())) > 0;
			}
		});
	}
}

TArray<FString> SUnityVersionControlBranchesWidget::GetSelectedBranches()
{
	TArray<FString> SelectedBranches;

	for (const FUnityVersionControlBranchRef& BranchRef : BranchesListView->GetSelectedItems())
	{
		SelectedBranches.Add(BranchRef->Name);
	}

	return SelectedBranches;
}

TSharedPtr<SWidget> SUnityVersionControlBranchesWidget::OnOpenContextMenu()
{
	const TArray<FString> SelectedBranches = GetSelectedBranches();
	const FString& SelectedBranch = SelectedBranches.Num() == 1 ? SelectedBranches[0] : FString();
	if (SelectedBranches.Num() == 0)
	{
		return nullptr;
	}
	const bool bSingleSelection = !SelectedBranch.IsEmpty();
	const bool bSingleNotCurrent = bSingleSelection && (SelectedBranch != CurrentBranchName);

	const bool bMergeXml = FUnityVersionControlModule::Get().GetProvider().GetPlasticScmVersion() >= UnityVersionControlVersions::MergeXml;

	static const FText SelectASingleBranchTooltip(LOCTEXT("SelectASingleBranchTooltip", "Select a single branch."));
	static const FText SelectADifferentBranchTooltip(LOCTEXT("SelectADifferentBranchTooltip", "Select a branch that is not the current one."));
	static const FText UpdateUVCSTooltip(LOCTEXT("MergeBranchXmlTooltip", "Update Unity Version Control (PlasticSCM) to 11.0.16.7726 or later."));

	UToolMenus* ToolMenus = UToolMenus::Get();
	static const FName MenuName = "UnityVersionControl.BranchesContextMenu";
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

	const FText CreateChildBranchTooltip(FText::Format(LOCTEXT("CreateChildBranchTooltip", "Create a child branch from {0}"),
		FText::FromString(SelectedBranch)));
	const FText& CreateChildBranchTooltipDynamic = bSingleSelection ? CreateChildBranchTooltip : SelectASingleBranchTooltip;
	Section.AddMenuEntry(
		TEXT("CreateChildBranch"),
		LOCTEXT("CreateChildBranch", "Create child branch..."),
		CreateChildBranchTooltipDynamic,
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateSP(this, &SUnityVersionControlBranchesWidget::OnCreateBranchClicked, SelectedBranch),
			FCanExecuteAction::CreateLambda([bSingleSelection]() { return bSingleSelection; })
		)
	);

	const FText SwitchToBranchTooltip(FText::Format(LOCTEXT("SwitchToBranchTooltip", "Switch the workspace to the branch {0}"),
		FText::FromString(SelectedBranch)));
	const FText& SwitchToBranchTooltipDynamic =
		bSingleNotCurrent ? SwitchToBranchTooltip :
		bSingleSelection ? SelectADifferentBranchTooltip : SelectASingleBranchTooltip;
	Section.AddMenuEntry(
		TEXT("SwitchToBranch"),
		LOCTEXT("SwitchToBranch", "Switch workspace to this branch"),
		SwitchToBranchTooltipDynamic,
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateSP(this, &SUnityVersionControlBranchesWidget::OnSwitchToBranchClicked, SelectedBranch),
			FCanExecuteAction::CreateLambda([bSingleNotCurrent]() { return bSingleNotCurrent; })
		)
	);

	Section.AddSeparator("PlasticSeparator1");

	const FText MergeBranchTooltip(FText::Format(LOCTEXT("MergeBranchTooltip", "Merge this branch {0} into the current branch {1}"),
		FText::FromString(SelectedBranch), FText::FromString(CurrentBranchName)));
	const FText& MergeBranchTooltipDynamic =
		!bMergeXml ? UpdateUVCSTooltip :
		bSingleNotCurrent ? MergeBranchTooltip :
		bSingleSelection ? SelectADifferentBranchTooltip : SelectASingleBranchTooltip;
	Section.AddMenuEntry(
		TEXT("MergeBranch"),
		LOCTEXT("MergeBranch", "Merge from this branch..."),
		MergeBranchTooltipDynamic,
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateSP(this, &SUnityVersionControlBranchesWidget::OnMergeBranchClicked, SelectedBranch),
			FCanExecuteAction::CreateLambda([bMergeXml, bSingleNotCurrent]() { return bMergeXml && bSingleNotCurrent; })
		)
	);

	Section.AddSeparator("PlasticSeparator2");

	const FText RenameBranchTooltip(FText::Format(LOCTEXT("RenameBranchTooltip", "Rename the branch {0}"),
		FText::FromString(SelectedBranch)));
	const FText& RenameBranchTooltipDynamic = bSingleSelection ? RenameBranchTooltip : SelectASingleBranchTooltip;
	Section.AddMenuEntry(
		TEXT("RenameBranch"),
		LOCTEXT("RenameBranch", "Rename..."),
		RenameBranchTooltipDynamic,
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateSP(this, &SUnityVersionControlBranchesWidget::OnRenameBranchClicked, SelectedBranch),
			FCanExecuteAction::CreateLambda([this, bSingleSelection]() { return bSingleSelection; })
		)
	);
	const FText DeleteBranchTooltip = bSingleSelection ?
		FText::Format(LOCTEXT("DeleteBranchTooltip", "Delete the branch {0}"), FText::FromString(SelectedBranch)) :
		LOCTEXT("DeleteBranchesTooltip", "Delete the selected branches.");
	Section.AddMenuEntry(
		TEXT("DeleteBranch"),
		LOCTEXT("DeleteBranch", "Delete"),
		DeleteBranchTooltip,
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateSP(this, &SUnityVersionControlBranchesWidget::OnDeleteBranchesClicked, SelectedBranches),
			FCanExecuteAction()
		)
	);

	return ToolMenus->GenerateWidget(Menu);
}


TSharedPtr<SWindow> SUnityVersionControlBranchesWidget::CreateDialogWindow(FText&& InTitle)
{
	return SNew(SWindow)
		.Title(InTitle)
		.HasCloseButton(true)
		.SupportsMaximize(false)
		.SupportsMinimize(false)
		.SizingRule(ESizingRule::Autosized)
		.AutoCenter(EAutoCenter::PreferredWorkArea);
}

void SUnityVersionControlBranchesWidget::OpenDialogWindow(TSharedPtr<SWindow>& InDialogWindowPtr)
{
	InDialogWindowPtr->SetOnWindowClosed(FOnWindowClosed::CreateSP(this, &SUnityVersionControlBranchesWidget::OnDialogClosed));

	TSharedPtr<SWindow> RootWindow = FGlobalTabmanager::Get()->GetRootWindow();
	FSlateApplication::Get().AddModalWindow(InDialogWindowPtr.ToSharedRef(), RootWindow);
}

void SUnityVersionControlBranchesWidget::OnDialogClosed(const TSharedRef<SWindow>& InWindow)
{
	DialogWindowPtr = nullptr;
}

void SUnityVersionControlBranchesWidget::OnCreateBranchClicked(FString InParentBranchName)
{
	// Create the branch modal dialog window (the frame for the content)
	DialogWindowPtr = CreateDialogWindow(LOCTEXT("PlasticCreateBranchTitle", "Create Branch"));

	// Setup its content widget, specific to the CreateBranch operation
	DialogWindowPtr->SetContent(SNew(SUnityVersionControlCreateBranch)
		.BranchesWidget(SharedThis(this))
		.ParentWindow(DialogWindowPtr)
		.ParentBranchName(InParentBranchName));

	OpenDialogWindow(DialogWindowPtr);
}

void SUnityVersionControlBranchesWidget::CreateBranch(const FString& InParentBranchName, const FString& InNewBranchName, const FString& InNewBranchComment, const bool bInSwitchWorkspace)
{
	if (!Notification.IsInProgress())
	{
		// Find and Unlink all loaded packages in Content directory to allow to update them
		PackageUtils::UnlinkPackages(PackageUtils::ListAllPackages());

		// Launch a custom "CreateBranch" operation
		FUnityVersionControlProvider& Provider = FUnityVersionControlModule::Get().GetProvider();
		TSharedRef<FPlasticCreateBranch, ESPMode::ThreadSafe> CreateBranchOperation = ISourceControlOperation::Create<FPlasticCreateBranch>();
		CreateBranchOperation->BranchName = InParentBranchName + TEXT("/") + InNewBranchName;
		CreateBranchOperation->Comment = InNewBranchComment;
		const ECommandResult::Type Result = Provider.Execute(CreateBranchOperation, TArray<FString>(), EConcurrency::Asynchronous, FSourceControlOperationComplete::CreateSP(this, &SUnityVersionControlBranchesWidget::OnCreateBranchOperationComplete, bInSwitchWorkspace));
		if (Result == ECommandResult::Succeeded)
		{
			// Display an ongoing notification during the whole operation (packages will be reloaded at the completion of the operation)
			Notification.DisplayInProgress(CreateBranchOperation->GetInProgressString());
			StartRefreshStatus();
		}
		else
		{
			// Report failure with a notification (but nothing need to be reloaded since no local change is expected)
			FNotification::DisplayFailure(CreateBranchOperation.Get());
		}
	}
	else
	{
		FMessageLog SourceControlLog("SourceControl");
		SourceControlLog.Warning(LOCTEXT("SourceControlMenu_InProgress", "Source control operation already in progress"));
		SourceControlLog.Notify();
	}
}

void SUnityVersionControlBranchesWidget::OnSwitchToBranchClicked(FString InBranchName)
{
	if (!Notification.IsInProgress())
	{
		// Warn the user about any unsaved assets (risk of losing work) but don't enforce saving them. Saving and checking out these assets will make the switch to the branch fail.
		PackageUtils::SaveDirtyPackages();

		// Find and Unlink all loaded packages in Content directory to allow to update them
		PackageUtils::UnlinkPackages(PackageUtils::ListAllPackages());

		// Launch a custom "Switch" operation
		FUnityVersionControlProvider& Provider = FUnityVersionControlModule::Get().GetProvider();
		TSharedRef<FPlasticSwitchToBranch, ESPMode::ThreadSafe> SwitchToBranchOperation = ISourceControlOperation::Create<FPlasticSwitchToBranch>();
		SwitchToBranchOperation->BranchName = InBranchName;
		const ECommandResult::Type Result = Provider.Execute(SwitchToBranchOperation, TArray<FString>(), EConcurrency::Asynchronous, FSourceControlOperationComplete::CreateSP(this, &SUnityVersionControlBranchesWidget::OnSwitchToBranchOperationComplete));
		if (Result == ECommandResult::Succeeded)
		{
			// Display an ongoing notification during the whole operation (packages will be reloaded at the completion of the operation)
			Notification.DisplayInProgress(SwitchToBranchOperation->GetInProgressString());
			StartRefreshStatus();
		}
		else
		{
			// Report failure with a notification (but nothing need to be reloaded since no local change is expected)
			FNotification::DisplayFailure(SwitchToBranchOperation.Get());
		}
	}
	else
	{
		FMessageLog SourceControlLog("SourceControl");
		SourceControlLog.Warning(LOCTEXT("SourceControlMenu_InProgress", "Source control operation already in progress"));
		SourceControlLog.Notify();
	}
}

void SUnityVersionControlBranchesWidget::OnMergeBranchClicked(FString InBranchName)
{
	const FText MergeBranchQuestion = FText::Format(LOCTEXT("MergeBranchDialog", "Merge branch {0} into the current branch {1}?"), FText::FromString(InBranchName), FText::FromString(CurrentBranchName));
	const EAppReturnType::Type Choice = FMessageDialog::Open(
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3
		EAppMsgCategory::Info,
#endif
		EAppMsgType::YesNo, MergeBranchQuestion
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3
		, LOCTEXT("MergeBranchTitle", "Merge Branch?")
#endif
	);
	if (Choice == EAppReturnType::Yes)
	{
		if (!Notification.IsInProgress())
		{
			// Warn the user about any unsaved assets (risk of losing work) but don't enforce saving them. Saving and checking out these assets might make the merge of the branch fail.
			PackageUtils::SaveDirtyPackages();

			// Find and Unlink all loaded packages in Content directory to allow to update them
			PackageUtils::UnlinkPackages(PackageUtils::ListAllPackages());

			// Launch a custom "Merge" operation
			FUnityVersionControlProvider& Provider = FUnityVersionControlModule::Get().GetProvider();
			TSharedRef<FPlasticMergeBranch, ESPMode::ThreadSafe> MergeBranchOperation = ISourceControlOperation::Create<FPlasticMergeBranch>();
			MergeBranchOperation->BranchName = InBranchName;
			const ECommandResult::Type Result = Provider.Execute(MergeBranchOperation, TArray<FString>(), EConcurrency::Asynchronous, FSourceControlOperationComplete::CreateSP(this, &SUnityVersionControlBranchesWidget::OnMergeBranchOperationComplete));
			if (Result == ECommandResult::Succeeded)
			{
				// Display an ongoing notification during the whole operation (packages will be reloaded at the completion of the operation)
				Notification.DisplayInProgress(MergeBranchOperation->GetInProgressString());
				StartRefreshStatus();
			}
			else
			{
				// Report failure with a notification (but nothing need to be reloaded since no local change is expected)
				FNotification::DisplayFailure(MergeBranchOperation.Get());
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

void SUnityVersionControlBranchesWidget::OnRenameBranchClicked(FString InBranchName)
{
	// Create the branch modal dialog window (the frame for the content)
	DialogWindowPtr = CreateDialogWindow(LOCTEXT("PlasticRenameBranchTitle", "Rename Branch"));

	// Setup its content widget, specific to the RenameBranch operation
	DialogWindowPtr->SetContent(SNew(SUnityVersionControlRenameBranch)
		.BranchesWidget(SharedThis(this))
		.ParentWindow(DialogWindowPtr)
		.OldBranchName(InBranchName));

	OpenDialogWindow(DialogWindowPtr);
}

void SUnityVersionControlBranchesWidget::RenameBranch(const FString& InOldBranchName, const FString& InNewBranchName)
{
	if (!Notification.IsInProgress())
	{
		// Launch a custom "RenameBranch" operation
		FUnityVersionControlProvider& Provider = FUnityVersionControlModule::Get().GetProvider();
		TSharedRef<FPlasticRenameBranch, ESPMode::ThreadSafe> RenameBranchOperation = ISourceControlOperation::Create<FPlasticRenameBranch>();
		RenameBranchOperation->OldName = InOldBranchName;
		RenameBranchOperation->NewName = InNewBranchName;
		const ECommandResult::Type Result = Provider.Execute(RenameBranchOperation, TArray<FString>(), EConcurrency::Asynchronous, FSourceControlOperationComplete::CreateSP(this, &SUnityVersionControlBranchesWidget::OnRenameBranchOperationComplete));
		if (Result == ECommandResult::Succeeded)
		{
			// Display an ongoing notification during the whole operation (packages will be reloaded at the completion of the operation)
			Notification.DisplayInProgress(RenameBranchOperation->GetInProgressString());
			StartRefreshStatus();
		}
		else
		{
			// Report failure with a notification (but nothing need to be reloaded since no local change is expected)
			FNotification::DisplayFailure(RenameBranchOperation.Get());
		}
	}
	else
	{
		FMessageLog SourceControlLog("SourceControl");
		SourceControlLog.Warning(LOCTEXT("SourceControlMenu_InProgress", "Source control operation already in progress"));
		SourceControlLog.Notify();
	}
}

void SUnityVersionControlBranchesWidget::OnDeleteBranchesClicked(TArray<FString> InBranchNames)
{
	// Create the branch modal dialog window (the frame for the content)
	DialogWindowPtr = CreateDialogWindow(LOCTEXT("PlasticDeleteBranchesTitle", "Delete Branches"));

	// Setup its content widget, specific to the DeleteBranches operation
	DialogWindowPtr->SetContent(SNew(SUnityVersionControlDeleteBranches)
		.BranchesWidget(SharedThis(this))
		.ParentWindow(DialogWindowPtr)
		.BranchNames(InBranchNames));

	OpenDialogWindow(DialogWindowPtr);
}

void SUnityVersionControlBranchesWidget::DeleteBranches(const TArray<FString>& InBranchNames)
{
	if (!Notification.IsInProgress())
	{
		// Launch a custom "DeleteBranches" operation
		FUnityVersionControlProvider& Provider = FUnityVersionControlModule::Get().GetProvider();
		TSharedRef<FPlasticDeleteBranches, ESPMode::ThreadSafe> DeleteBranchesOperation = ISourceControlOperation::Create<FPlasticDeleteBranches>();
		DeleteBranchesOperation->BranchNames = InBranchNames;
		const ECommandResult::Type Result = Provider.Execute(DeleteBranchesOperation, TArray<FString>(), EConcurrency::Asynchronous, FSourceControlOperationComplete::CreateSP(this, &SUnityVersionControlBranchesWidget::OnDeleteBranchesOperationComplete));
		if (Result == ECommandResult::Succeeded)
		{
			// Display an ongoing notification during the whole operation (packages will be reloaded at the completion of the operation)
			Notification.DisplayInProgress(DeleteBranchesOperation->GetInProgressString());
			StartRefreshStatus();
		}
		else
		{
			// Report failure with a notification (but nothing need to be reloaded since no local change is expected)
			FNotification::DisplayFailure(DeleteBranchesOperation.Get());
		}
	}
	else
	{
		FMessageLog SourceControlLog("SourceControl");
		SourceControlLog.Warning(LOCTEXT("SourceControlMenu_InProgress", "Source control operation already in progress"));
		SourceControlLog.Notify();
	}
}

void SUnityVersionControlBranchesWidget::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
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
		RequestBranchesRefresh();
		bShouldRefresh = false;
	}

	if (bIsRefreshing)
	{
		TickRefreshStatus(InDeltaTime);
	}
}

bool SUnityVersionControlBranchesWidget::IsBranchNameValid(const FString& InBranchName)
{
	// Branch name cannot contain any of the following characters:
	// Note: tabs are technically not forbidden in branch names, but having one at the end doesn't work as expected
	// (it is trimmed at creation, so the switch to the new branch fails)
	static const FString BranchNameInvalidChars(TEXT("@#/:\"?'\n\r\t"));

	for (TCHAR Char : InBranchName)
	{
		int32 Index;
		if (BranchNameInvalidChars.FindChar(Char, Index))
		{
			return false;
		}
	}

	return true;
}

void SUnityVersionControlBranchesWidget::StartRefreshStatus()
{
	if (!bIsRefreshing)
	{
		bIsRefreshing = true;
		RefreshStatusStartSecs = FPlatformTime::Seconds();
	}
}

void SUnityVersionControlBranchesWidget::TickRefreshStatus(double InDeltaTime)
{
	const int32 RefreshStatusTimeElapsed = static_cast<int32>(FPlatformTime::Seconds() - RefreshStatusStartSecs);
	RefreshStatus = FText::Format(LOCTEXT("UnityVersionControl_RefreshBranches", "Refreshing branches... ({0} s)"), FText::AsNumber(RefreshStatusTimeElapsed));
}

void SUnityVersionControlBranchesWidget::EndRefreshStatus()
{
	bIsRefreshing = false;
	RefreshStatus = FText::GetEmpty();
}

void SUnityVersionControlBranchesWidget::RequestBranchesRefresh()
{
	if (!ISourceControlModule::Get().IsEnabled() || (!FUnityVersionControlModule::Get().GetProvider().IsAvailable()))
	{
		return;
	}

	StartRefreshStatus();

	TSharedRef<FPlasticGetBranches, ESPMode::ThreadSafe> GetBranchesOperation = ISourceControlOperation::Create<FPlasticGetBranches>();
	if (FromDateInDays > -1)
	{
		GetBranchesOperation->FromDate = FDateTime::Now() - FTimespan::FromDays(FromDateInDays);
	}

	FUnityVersionControlProvider& Provider = FUnityVersionControlModule::Get().GetProvider();
	Provider.Execute(GetBranchesOperation, EConcurrency::Asynchronous, FSourceControlOperationComplete::CreateSP(this, &SUnityVersionControlBranchesWidget::OnGetBranchesOperationComplete));
}

void SUnityVersionControlBranchesWidget::OnGetBranchesOperationComplete(const FSourceControlOperationRef& InOperation, ECommandResult::Type InResult)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(SUnityVersionControlBranchesWidget::OnGetBranchesOperationComplete);

	TSharedRef<FPlasticGetBranches, ESPMode::ThreadSafe> OperationGetBranches = StaticCastSharedRef<FPlasticGetBranches>(InOperation);
	SourceControlBranches = MoveTemp(OperationGetBranches->Branches);

	CurrentBranchName = FUnityVersionControlModule::Get().GetProvider().GetBranchName();

	EndRefreshStatus();
	OnRefreshUI();
}

void SUnityVersionControlBranchesWidget::OnCreateBranchOperationComplete(const FSourceControlOperationRef& InOperation, ECommandResult::Type InResult, const bool bInSwitchWorkspace)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(SUnityVersionControlBranchesWidget::OnCreateBranchOperationComplete);

	Notification.RemoveInProgress();

	FNotification::DisplayResult(InOperation, InResult);

	if (InResult == ECommandResult::Succeeded)
	{
		if (bInSwitchWorkspace)
		{
			TSharedRef<FPlasticCreateBranch, ESPMode::ThreadSafe> CreateBranchOperation = StaticCastSharedRef<FPlasticCreateBranch>(InOperation);
			OnSwitchToBranchClicked(CreateBranchOperation->BranchName);
		}
		else
		{
			// Ask for a full refresh of the list of branches (and don't call EndRefreshStatus() yet)
			bShouldRefresh = true;
		}
	}
	else
	{
		EndRefreshStatus();
	}
}

void SUnityVersionControlBranchesWidget::OnSwitchToBranchOperationComplete(const FSourceControlOperationRef& InOperation, ECommandResult::Type InResult)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(SUnityVersionControlBranchesWidget::OnSwitchToBranchOperationComplete);

	// Reload packages that where updated by the SwitchToBranch operation (and the current map if needed)
	TSharedRef<FPlasticSwitchToBranch, ESPMode::ThreadSafe> SwitchToBranchOperation = StaticCastSharedRef<FPlasticSwitchToBranch>(InOperation);
	PackageUtils::ReloadPackages(SwitchToBranchOperation->UpdatedFiles);

	// Ask for a full refresh of the list of branches (and don't call EndRefreshStatus() yet)
	bShouldRefresh = true;

	Notification.RemoveInProgress();

	FNotification::DisplayResult(InOperation, InResult);
}

void SUnityVersionControlBranchesWidget::OnMergeBranchOperationComplete(const FSourceControlOperationRef& InOperation, ECommandResult::Type InResult)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(SUnityVersionControlBranchesWidget::OnMergeBranchOperationComplete);

	// Reload packages that where updated by the MergeBranch operation (and the current map if needed)
	TSharedRef<FPlasticMergeBranch, ESPMode::ThreadSafe> MergeBranchOperation = StaticCastSharedRef<FPlasticMergeBranch>(InOperation);
	PackageUtils::ReloadPackages(MergeBranchOperation->UpdatedFiles);

	Notification.RemoveInProgress();

	FNotification::DisplayResult(InOperation, InResult);

	EndRefreshStatus();
}

void SUnityVersionControlBranchesWidget::OnRenameBranchOperationComplete(const FSourceControlOperationRef& InOperation, ECommandResult::Type InResult)
{
	// Ask for a full refresh of the list of branches (and don't call EndRefreshStatus() yet)
	bShouldRefresh = true;

	Notification.RemoveInProgress();

	FNotification::DisplayResult(InOperation, InResult);
}

void SUnityVersionControlBranchesWidget::OnDeleteBranchesOperationComplete(const FSourceControlOperationRef& InOperation, ECommandResult::Type InResult)
{
	// Ask for a full refresh of the list of branches (and don't call EndRefreshStatus() yet)
	bShouldRefresh = true;

	Notification.RemoveInProgress();

	FNotification::DisplayResult(InOperation, InResult);
}

void SUnityVersionControlBranchesWidget::OnSourceControlProviderChanged(ISourceControlProvider& OldProvider, ISourceControlProvider& NewProvider)
{
	bSourceControlAvailable = NewProvider.IsAvailable(); // Check if it is connected.
	bShouldRefresh = true;

	if (&NewProvider != &OldProvider)
	{
		BranchRows.Reset();
		if (GetListView())
		{
			GetListView()->RequestListRefresh();
		}
	}
}

FReply SUnityVersionControlBranchesWidget::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::F5)
	{
		// Pressing F5 refreshes the list of branches
		RequestBranchesRefresh();
		return FReply::Handled();
	}
	else if (InKeyEvent.GetKey() == EKeys::Enter)
	{
		// Pressing Enter switches to the selected branch.
		const TArray<FString> SelectedBranches = GetSelectedBranches();
		if (SelectedBranches.Num() == 1)
		{
			const FString& SelectedBranch = SelectedBranches[0];
			// Note: this action require a confirmation dialog (while the Delete below already have one in OnDeleteBranchesClicked()).
			const FText SwitchToBranchQuestion = FText::Format(LOCTEXT("SwitchToBranchDialog", "Switch workspace to branch {0}?"), FText::FromString(SelectedBranch));
			const EAppReturnType::Type Choice = FMessageDialog::Open(
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3
				EAppMsgCategory::Info,
#endif
				EAppMsgType::YesNo, SwitchToBranchQuestion
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3
				, LOCTEXT("SwitchToBranchTitle", "Switch Branch?")
#endif
			);
			if (Choice == EAppReturnType::Yes)
			{
				OnSwitchToBranchClicked(SelectedBranch);
			}
		}
		return FReply::Handled();
	}
	else if (InKeyEvent.GetKey() == EKeys::F2)
	{
		// Pressing F2 renames the selected branches
		const TArray<FString> SelectedBranches = GetSelectedBranches();
		if (SelectedBranches.Num() == 1)
		{
			const FString& SelectedBranch = SelectedBranches[0];
			OnRenameBranchClicked(SelectedBranch);
		}
		return FReply::Handled();
	}
	else if ((InKeyEvent.GetKey() == EKeys::Delete) || (InKeyEvent.GetKey() == EKeys::BackSpace))
	{
		// Pressing Delete or BackSpace deletes the selected branches
		const TArray<FString> SelectedBranches = GetSelectedBranches();
		if (SelectedBranches.Num() > 0)
		{
			OnDeleteBranchesClicked(SelectedBranches);
		}
		return FReply::Handled();
	}

	return FReply::Unhandled();
}

#undef LOCTEXT_NAMESPACE
