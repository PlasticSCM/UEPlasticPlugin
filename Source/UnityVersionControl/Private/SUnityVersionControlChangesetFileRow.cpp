// Copyright (c) 2025 Unity Technologies

#include "SUnityVersionControlChangesetFileRow.h"

#include "UnityVersionControlState.h"
#include "UnityVersionControlUtils.h"

#include "Misc/Paths.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Images/SLayeredImage.h"
#include "Widgets/Text/STextBlock.h"

#include "Runtime/Launch/Resources/Version.h"
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
#include "Styling/AppStyle.h"
#else
#include "EditorStyleSet.h"
#endif

#define LOCTEXT_NAMESPACE "UnityVersionControlChangesetFileWindow"

FName UnityVersionControlChangesetFilesListViewColumn::Icon::Id() { return TEXT("Icon"); }
FText UnityVersionControlChangesetFilesListViewColumn::Icon::GetDisplayText() { return LOCTEXT("Icon_Column", "Revision Control Status"); }
FText UnityVersionControlChangesetFilesListViewColumn::Icon::GetToolTipText() { return LOCTEXT("Icon_Column_Tooltip", "Icon displaying the type of change"); }

FName UnityVersionControlChangesetFilesListViewColumn::Name::Id() { return TEXT("Name"); }
FText UnityVersionControlChangesetFilesListViewColumn::Name::GetDisplayText() { return LOCTEXT("Name_Column", "Name"); }
FText UnityVersionControlChangesetFilesListViewColumn::Name::GetToolTipText() { return LOCTEXT("Name_Column_Tooltip", "Name of the file"); }

FName UnityVersionControlChangesetFilesListViewColumn::Path::Id() { return TEXT("Path"); }
FText UnityVersionControlChangesetFilesListViewColumn::Path::GetDisplayText() { return LOCTEXT("Path_Column", "Path"); }
FText UnityVersionControlChangesetFilesListViewColumn::Path::GetToolTipText() { return LOCTEXT("Path_Column_Tooltip", "Path of the file relative to the workspace"); }

void SUnityVersionControlChangesetFileRow::Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwner)
{
	FileToVisualize = InArgs._FileToVisualize.Get();
	HighlightText = InArgs._HighlightText;

	FSuperRowType::FArguments Args = FSuperRowType::FArguments()
		.ShowSelection(true);
	FSuperRowType::Construct(Args, InOwner);
}

TSharedRef<SWidget> GetSCCFileWidget(FUnityVersionControlState* InFileState)
{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
	const FSlateBrush* IconBrush = FAppStyle::GetBrush("ContentBrowser.ColumnViewAssetIcon");
#else
	const FSlateBrush* IconBrush = FEditorStyle::GetBrush("ContentBrowser.ColumnViewAssetIcon");
#endif

	// Make icon overlays (eg, SCC and dirty status) a reasonable size in relation to the icon size (note: it is assumed this icon is square)
	const float ICON_SCALING_FACTOR = 0.7f;
	const float IconOverlaySize = static_cast<float>(IconBrush->ImageSize.X * ICON_SCALING_FACTOR);

	return SNew(SOverlay)
		// The actual icon
		+SOverlay::Slot()
		[
			SNew(SImage)
			.Image(IconBrush)
		]
		// Source control state
		+SOverlay::Slot()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Top)
		[
			SNew(SBox)
			.WidthOverride(IconOverlaySize)
			.HeightOverride(IconOverlaySize)
			[
#if ENGINE_MAJOR_VERSION >= 5
				SNew(SLayeredImage, InFileState->GetIcon())
#else
				SNew(SLayeredImage, FEditorStyle::GetBrush(InFileState->GetSmallIconName()), FSlateColor())
#endif
				.ToolTipText(InFileState->GetDisplayTooltip())
			]
		];
}


TSharedRef<SWidget> SUnityVersionControlChangesetFileRow::GenerateWidgetForColumn(const FName& InColumnId)
{
	if (InColumnId == UnityVersionControlChangesetFilesListViewColumn::Icon::Id())
	{
		return SNew(SBox)
			.WidthOverride(16.f) // Small Icons are usually 16x16
			.ToolTipText(FileToVisualize->ToText())
			.HAlign(HAlign_Center)
			[
				GetSCCFileWidget(FileToVisualize)
			];
	}
	else if (InColumnId == UnityVersionControlChangesetFilesListViewColumn::Name::Id())
	{
		return SNew(STextBlock)
			.Text(FText::FromString(FPaths::GetBaseFilename(FileToVisualize->LocalFilename, true))) // Just the name without its path or extension
			.ToolTipText(FText::FromString(FPaths::GetCleanFilename(FileToVisualize->LocalFilename))) // Name with extension
			.HighlightText(HighlightText);
	}
	else if (InColumnId == UnityVersionControlChangesetFilesListViewColumn::Path::Id())
	{
		return SNew(STextBlock)
			.Text(FText::FromString(FPaths::GetBaseFilename(FileToVisualize->LocalFilename, false))) // Relative Path without the extension
			.ToolTipText(FText::FromString(FileToVisualize->LocalFilename))
			.HighlightText(HighlightText);
	}
	else
	{
		return SNullWidget::NullWidget;
	}
}

#undef LOCTEXT_NAMESPACE
