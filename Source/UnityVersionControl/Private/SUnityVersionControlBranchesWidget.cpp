// Copyright (c) 2023 Unity Technologies

#include "SUnityVersionControlBranchesWidget.h"

#include "UnityVersionControlModule.h"
#include "UnityVersionControlOperations.h"
#include "UnityVersionControlProjectSettings.h"
#include "UnityVersionControlBranch.h"
#include "SUnityVersionControlBranchRow.h"
#include "SUnityVersionControlCreateBranch.h"

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
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/SHeaderRow.h"
#include "Widgets/SWindow.h"

#define LOCTEXT_NAMESPACE "UnityVersionControlWindow"

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
			.Padding(4)
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
				.MaxWidth(10)
				[
					SNew(SSpacer)
				]
				+SHorizontalBox::Slot()
				.VAlign(VAlign_Center)
				.MaxWidth(300)
				[
					SAssignNew(FileSearchBox, SSearchBox)
					.HintText(LOCTEXT("SearchBranches", "Search Branches"))
					.ToolTipText(LOCTEXT("PlasticBranchesSearch_Tooltip", "Filter the list of branches by keyword."))
					.OnTextChanged(this, &SUnityVersionControlBranchesWidget::OnSearchTextChanged)
				]
				+SHorizontalBox::Slot()
				.VAlign(VAlign_Center)
				.MaxWidth(125)
				.Padding(FMargin(10.f, 0.f))
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
		.SelectionMode(ESelectionMode::Single)
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
		Settings->bShowBranchRepositoryColumn = false;
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

FString SUnityVersionControlBranchesWidget::GetSelectedBranch()
{
	for (const FUnityVersionControlBranchPtr& BranchPtr : BranchesListView->GetSelectedItems())
	{
		return BranchPtr->Name;
	}

	return FString();
}

TSharedPtr<SWidget> SUnityVersionControlBranchesWidget::OnOpenContextMenu()
{
	const FString SelectedBranch = GetSelectedBranch();
	if (SelectedBranch.IsEmpty())
	{
		return nullptr;
	}

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

	Section.AddMenuEntry(
		TEXT("CreateChildBranch"),
		LOCTEXT("CreateChildBranch", "Create child branch"),
		LOCTEXT("CreateChildBranchTooltip", "Create child branch from the selected branch."),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateSP(this, &SUnityVersionControlBranchesWidget::OnCreateBranchClicked, SelectedBranch)
		)
	);

	Section.AddMenuEntry(
		TEXT("SwitchToBranch"),
		LOCTEXT("SwitchToBranch", "Switch workspace to this branch"),
		LOCTEXT("SwitchToBranchTooltip", "Switch workspace to this branch."),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateSP(this, &SUnityVersionControlBranchesWidget::OnSwitchToBranchClicked, SelectedBranch),
			FCanExecuteAction::CreateLambda([this, SelectedBranch]() { return SelectedBranch != CurrentBranchName; })
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
		.SizingRule(ESizingRule::Autosized);
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
		const bool bSaved = PackageUtils::SaveDirtyPackages();
		if (bSaved || bInSwitchWorkspace)
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
				FNotification::DisplayFailure(CreateBranchOperation->GetName());
			}
		}
		else
		{
			FMessageLog SourceControlLog("SourceControl");
			SourceControlLog.Warning(LOCTEXT("SourceControlMenu_Switch_Unsaved", "Save All Assets before attempting to Switch workspace to a branch!"));
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

void SUnityVersionControlBranchesWidget::OnSwitchToBranchClicked(FString InBranchName)
{
	if (!Notification.IsInProgress())
	{
		const bool bSaved = PackageUtils::SaveDirtyPackages();
		if (bSaved)
		{
			// Find and Unlink all loaded packages in Content directory to allow to update them
			PackageUtils::UnlinkPackages(PackageUtils::ListAllPackages());

			// Launch a custom "SwitchToBranch" operation
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
				FNotification::DisplayFailure(SwitchToBranchOperation->GetName());
			}
		}
		else
		{
			FMessageLog SourceControlLog("SourceControl");
			SourceControlLog.Warning(LOCTEXT("SourceControlMenu_Switch_Unsaved", "Save All Assets before attempting to Switch workspace to a branch!"));
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

	if (InResult == ECommandResult::Succeeded)
	{
		FNotification::DisplaySuccess(InOperation->GetName());

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
		FNotification::DisplayFailure(InOperation->GetName());

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

	// Report result with a notification
	if (InResult == ECommandResult::Succeeded)
	{
		FNotification::DisplaySuccess(InOperation->GetName());
	}
	else
	{
		FNotification::DisplayFailure(InOperation->GetName());
	}
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

#undef LOCTEXT_NAMESPACE
