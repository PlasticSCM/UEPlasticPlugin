// Copyright (c) 2023 Unity Technologies

#include "UnityVersionControlWindow.h"

#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"

#include "UnityVersionControlStyle.h"

static const FName UnityVersionControlWindowTabName("UnityVersionControlWindow");

#define LOCTEXT_NAMESPACE "UnityVersionControlWindow"

// POC context menu
const FName DerivedDataCacheStatisticsContextMenu = TEXT("DerivedDataCacheStatistics.ContextMenu");


void FUnityVersionControlWindow::Register()
{
	FUnityVersionControlStyle::Initialize();
	FUnityVersionControlStyle::ReloadTextures();

	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(UnityVersionControlWindowTabName, FOnSpawnTab::CreateRaw(this, &FUnityVersionControlWindow::OnSpawnTab))
		.SetDisplayName(LOCTEXT("UnityVersionControlWindowTabTitle", "Unity Version Control"))
		.SetMenuType(ETabSpawnerMenuType::Hidden)
	.SetIcon(FSlateIcon(FUnityVersionControlStyle::Get().GetStyleSetName(), "UnityVersionControl.PluginIcon.Small"));
}

void FUnityVersionControlWindow::Unregister()
{
	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(UnityVersionControlWindowTabName);

	FUnityVersionControlStyle::Shutdown();
}

TSharedRef<SDockTab> FUnityVersionControlWindow::OnSpawnTab(const FSpawnTabArgs& SpawnTabArgs)
{
	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			CreateCacheBranchesWidget().ToSharedRef()
		];
}

void FUnityVersionControlWindow::OpenTab()
{
	FGlobalTabmanager::Get()->TryInvokeTab(UnityVersionControlWindowTabName);
}

TSharedPtr<SWidget> FUnityVersionControlWindow::CreateCacheBranchesWidget()
{
	return SNew(SBranchesWidget);
}

void SBranchesWidget::Construct(const FArguments& InArgs)
{
	if (!UToolMenus::Get()->IsMenuRegistered(DerivedDataCacheStatisticsContextMenu))
	{
		UToolMenu* ContextMenu = UToolMenus::Get()->RegisterMenu(DerivedDataCacheStatisticsContextMenu);
		ContextMenu->bShouldCloseWindowAfterMenuSelection = true;

		ContextMenu->AddDynamicSection(NAME_None, FNewToolMenuDelegate::CreateStatic(&SBranchesWidget::CreatePOCMenu));
	}

	const float RowMargin = 0.0f;
	const float TitleMargin = 10.0f;
	const float ColumnMargin = 10.0f;
	const FSlateColor TitleColor = FStyleColors::AccentWhite;
	const FSlateFontInfo TitleFont = FCoreStyle::GetDefaultFontStyle("Bold", 10);

	this->ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SHorizontalBox)
			// TODO: how to add a search icon?
			+SHorizontalBox::Slot()
			[
				// TODO: Filter (like the Desktop app) or Search (like the Unity Plugin)?
				SNew(SEditableTextBox)
				.Justification(ETextJustify::Left)
				.HintText(LOCTEXT("Filter", "Filter"))
				.OnTextChanged(this, &SBranchesWidget::OnFilterTextChanged)
				.OnContextMenuOpening(this, &SBranchesWidget::OnCreateContextMenu)
			]
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0, 5, 0, 0)
		.Expose(GridSlot)
		[
			GetGridPanel()
		]
	];

	RegisterActiveTimer(60.f, FWidgetActiveTimerDelegate::CreateSP(this, &SBranchesWidget::UpdateGridPanels));
}

void SBranchesWidget::OnFilterTextChanged(const FText& SearchText)
{
	FilterText = SearchText.ToString();

	UpdateGridPanels(0.0, 0.0f);
}


EActiveTimerReturnType SBranchesWidget::UpdateGridPanels(double InCurrentTime, float InDeltaTime)
{
	(*GridSlot)
	[
		GetGridPanel()
	];

	SlatePrepass(GetPrepassLayoutScaleMultiplier());

	return EActiveTimerReturnType::Continue;
}

TSharedRef<SWidget> SBranchesWidget::GetGridPanel()
{
	TSharedRef<SGridPanel> Panel =
		SNew(SGridPanel);

	const float RowMargin = 0.0f;
	const float ColumnMargin = 10.0f;
	const FSlateColor TitleColor = FStyleColors::AccentWhite;
	const FSlateFontInfo TitleFont = FCoreStyle::GetDefaultFontStyle("Bold", 10);
	const FMargin DefaultMarginFirstColumn(ColumnMargin, RowMargin);

	int32 Row = 0;

	const FMargin TitleMargin(0.0f, 10.0f, ColumnMargin, 10.0f);
	const FMargin TitleMarginFirstColumn(ColumnMargin, 10.0f);
	const FMargin DefaultMargin(0.0f, RowMargin, ColumnMargin, RowMargin);
	
	Panel->AddSlot(0, Row)
	[
		SNew(STextBlock)
	];

	Panel->AddSlot(1, Row)
	[
		SNew(STextBlock)
		.Margin(TitleMarginFirstColumn)
		.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
		.ColorAndOpacity(TitleColor)
		.Text(LOCTEXT("BranchName", "Name"))
	];

	Panel->AddSlot(2, Row)
	[
		SNew(STextBlock)
		.Margin(TitleMargin)
		.ColorAndOpacity(TitleColor)
		.Font(TitleFont)
		.Text(LOCTEXT("CreateBy", "Created By"))
	];

	Panel->AddSlot(3, Row)
	[
		SNew(STextBlock)
		.Margin(TitleMargin)
		.ColorAndOpacity(TitleColor)
		.Font(TitleFont)
		.Text(LOCTEXT("CreationDate", "Creation date"))
	];

	Panel->AddSlot(4, Row)
	[
		SNew(STextBlock)
		.Margin(TitleMargin)
		.ColorAndOpacity(TitleColor)
		.Font(TitleFont)
		.Text(LOCTEXT("Comment", "Comment"))
	];


	// POC
	for (int32 i = 0; i < 10; i++)
	{
		// POC generate branch names
		const FString BranchName = (i == 0) ? FString(TEXT("/main")) : FString::Printf(TEXT("/main/scm%d"), 100271 + (i * i));
		const FString BranchCreatedBy = TEXT("sebastien.rombauts@unity3d.com");
		const FString BranchCreationDate = TEXT("23/10/2023 14:24:14");
		const FString BranchComment = FString::Printf(TEXT("Proof of Concept comment for branch %s"), *BranchName);

		// Filter branches on name, auther and comment
		if ((BranchName.Find(FilterText) == INDEX_NONE) && (BranchCreatedBy.Find(FilterText) == INDEX_NONE) && (BranchComment.Find(FilterText) == INDEX_NONE))
		{
			continue;
		}

		Row++;

		Panel->AddSlot(0, Row)
		[
			SNew(STextBlock)
		];

		Panel->AddSlot(1, Row)
		.HAlign(HAlign_Left)
		[
			SNew(STextBlock)
			.Margin(DefaultMargin)
			.Text(FText::FromString(BranchName))
		];

		Panel->AddSlot(2, Row)
		.HAlign(HAlign_Left)
		[
			SNew(STextBlock)
			.Margin(DefaultMargin)
			.Text(FText::FromString(BranchCreatedBy))
		];

		Panel->AddSlot(3, Row)
		.HAlign(HAlign_Left)
		[
			SNew(STextBlock)
			.Margin(DefaultMargin)
			.Text(FText::FromString(BranchCreationDate))
		];

		Panel->AddSlot(4, Row)
		.HAlign(HAlign_Left)
		[
			SNew(STextBlock)
			.Margin(DefaultMargin)
			.Text(FText::FromString(BranchComment))
		];
	}

	return Panel;
}

void SBranchesWidget::CreatePOCMenu(UToolMenu* InToolMenu)
{
	UBranchesWidgetContext* FoundContext = InToolMenu->FindContext<UBranchesWidgetContext>();
	if (!FoundContext)
	{
		return;
	}

	TSharedPtr<SBranchesWidget> BranchesWidget = FoundContext->BranchesWidget.Pin();
	if (!BranchesWidget)
	{
		return;
	}

	FToolMenuSection& Section = InToolMenu->AddSection("Section");
	Section.AddMenuEntry(
		TEXT("CreateChildBranch"),
		LOCTEXT("CreateChildBranch", "Create child branch"),
		LOCTEXT("CreateChildBranchTooltip", "Create child branch."),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateSP(BranchesWidget.Get(), &SBranchesWidget::OnPOCMenuAction)
		)
	);
	Section.AddMenuEntry(
		TEXT("SwitchToBranch"),
		LOCTEXT("SwitchTo", "Switch workspace to this branch"),
		LOCTEXT("SwitchToTooltip", "Switch workspace to this branch."),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateSP(BranchesWidget.Get(), &SBranchesWidget::OnPOCMenuAction)
		)
	);

	Section.AddSeparator("PlasticSeparator");

	Section.AddMenuEntry(
		TEXT("RenameBranch"),
		LOCTEXT("RenameBranch", "Rename branch"),
		LOCTEXT("RenameBranchTooltip", "Rename branch."),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateSP(BranchesWidget.Get(), &SBranchesWidget::OnPOCMenuAction)
		)
	);
	Section.AddMenuEntry(
		TEXT("DeleteBranch"),
		LOCTEXT("DeleteBranch", "Delete branch"),
		LOCTEXT("DeleteBranchTooltip", "Delete branch."),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateSP(BranchesWidget.Get(), &SBranchesWidget::OnPOCMenuAction)
		)
	);

}

void SBranchesWidget::OnPOCMenuAction()
{
}

TSharedPtr<SWidget> SBranchesWidget::OnCreateContextMenu()
{
	// Provide the Menu with a Context to reference the current widget
	FToolMenuContext Context;
	UBranchesWidgetContext* BranchesWidgetContext = NewObject<UBranchesWidgetContext>();
	BranchesWidgetContext->BranchesWidget = SharedThis(this);
	Context.AddObject(BranchesWidgetContext);

	if (UToolMenu* GeneratedContextMenu = UToolMenus::Get()->GenerateMenu(DerivedDataCacheStatisticsContextMenu, Context))
	{
		return UToolMenus::Get()->GenerateWidget(GeneratedContextMenu);
	}

	return SNullWidget::NullWidget;
}

#undef LOCTEXT_NAMESPACE
