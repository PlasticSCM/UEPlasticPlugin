// Copyright (c) 2023 Unity Technologies

#include "UnityVersionControlWindow.h"

#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"

#include "UnityVersionControlStyle.h"

static const FName UnityVersionControlWindowTabName("UnityVersionControlWindow");

#define LOCTEXT_NAMESPACE "UnityVersionControlWindow"

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

TSharedPtr<SWidget> FUnityVersionControlWindow::CreateCacheStatisticsDialog()
{
	return SNew(SDerivedDataCacheStatisticsDialog);
}

TSharedRef<SDockTab> FUnityVersionControlWindow::OnSpawnTab(const FSpawnTabArgs& SpawnTabArgs)
{
	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			CreateCacheStatisticsDialog().ToSharedRef()
		];
}

void FUnityVersionControlWindow::OpenTab()
{
	FGlobalTabmanager::Get()->TryInvokeTab(UnityVersionControlWindowTabName);
}


void SDerivedDataCacheStatisticsDialog::Construct(const FArguments& InArgs)
{
	const float RowMargin = 0.0f;
	const float TitleMargin = 10.0f;
	const float ColumnMargin = 10.0f;
	const FSlateColor TitleColor = FStyleColors::AccentWhite;
	const FSlateFontInfo TitleFont = FCoreStyle::GetDefaultFontStyle("Bold", 10);

	this->ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.Padding(0, 20, 0, 0)
		.AutoHeight()
		[
			SNew(SHorizontalBox)
			+SHorizontalBox::Slot()
			.FillWidth(1.0f)
			[
				SNew(STextBlock)
				.Margin(TitleMargin)
				.ColorAndOpacity(TitleColor)
				.Font(TitleFont)
				.Justification(ETextJustify::Left)
				.Text(LOCTEXT("UnityVersionControl_Branches", "Branches"))
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

	RegisterActiveTimer(0.5f, FWidgetActiveTimerDelegate::CreateSP(this, &SDerivedDataCacheStatisticsDialog::UpdateGridPanels));
}

EActiveTimerReturnType SDerivedDataCacheStatisticsDialog::UpdateGridPanels(double InCurrentTime, float InDeltaTime)
{
	(*GridSlot)
	[
		GetGridPanel()
	];

	SlatePrepass(GetPrepassLayoutScaleMultiplier());

	return EActiveTimerReturnType::Continue;
}


TSharedRef<SWidget> SDerivedDataCacheStatisticsDialog::GetGridPanel()
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
	for (int32 i = 0; i < 3; i++)
	{
		Row++;

		Panel->AddSlot(0, Row)
		[
			SNew(STextBlock)
		];

		Panel->AddSlot(1, Row)
		.HAlign(HAlign_Right)
		[
			SNew(STextBlock)
			.Margin(DefaultMargin)
			.Text(FText::FromString(TEXT("/main")))
		];

		Panel->AddSlot(2, Row)
		.HAlign(HAlign_Right)
		[
			SNew(STextBlock)
			.Margin(DefaultMargin)
			.Text(FText::FromString(TEXT("sebastien.rombauts@unity3d.com")))
		];

		Panel->AddSlot(3, Row)
		.HAlign(HAlign_Right)
		[
			SNew(STextBlock)
			.Margin(DefaultMargin)
			.Text(FText::FromString(TEXT("23/10/2023 14:24:14")))
		];

		Panel->AddSlot(4, Row)
		.HAlign(HAlign_Right)
		[
			SNew(STextBlock)
			.Margin(DefaultMargin)
			.Text(FText::FromString(TEXT("Proof of concept branch")))
		];
	}

	return Panel;
}

#undef LOCTEXT_NAMESPACE
