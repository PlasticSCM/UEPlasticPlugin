// Copyright (c) 2025 Unity Technologies

#include "SUnityVersionControlChangesetRow.h"

#include "UnityVersionControlChangeset.h"
#include "UnityVersionControlUtils.h"

#include "Widgets/Text/STextBlock.h"

#include "Runtime/Launch/Resources/Version.h"
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
#include "Styling/AppStyle.h"
#else
#include "EditorStyleSet.h"
#endif

#define LOCTEXT_NAMESPACE "UnityVersionControlChangesetWindow"

FName UnityVersionControlChangesetsListViewColumn::ChangesetId::Id() { return TEXT("ChangesetId"); }
FText UnityVersionControlChangesetsListViewColumn::ChangesetId::GetDisplayText() { return LOCTEXT("ChangesetId_Column", "Name"); }
FText UnityVersionControlChangesetsListViewColumn::ChangesetId::GetToolTipText() { return LOCTEXT("ChangesetId_Column_Tooltip", "Id of the changeset"); }

FName UnityVersionControlChangesetsListViewColumn::CreatedBy::Id() { return TEXT("CreatedBy"); }
FText UnityVersionControlChangesetsListViewColumn::CreatedBy::GetDisplayText() { return LOCTEXT("CreatedBy_Column", "Created by"); }
FText UnityVersionControlChangesetsListViewColumn::CreatedBy::GetToolTipText() { return LOCTEXT("CreatedBy_Column_Tooltip", "Creator of the changeset"); }

FName UnityVersionControlChangesetsListViewColumn::Date::Id() { return TEXT("Date"); }
FText UnityVersionControlChangesetsListViewColumn::Date::GetDisplayText() { return LOCTEXT("Date_Column", "Creation date"); }
FText UnityVersionControlChangesetsListViewColumn::Date::GetToolTipText() { return LOCTEXT("Date_Column_Tooltip", "Date of creation of the changeset"); }

FName UnityVersionControlChangesetsListViewColumn::Comment::Id() { return TEXT("Comment"); }
FText UnityVersionControlChangesetsListViewColumn::Comment::GetDisplayText() { return LOCTEXT("Comment_Column", "Comment"); }
FText UnityVersionControlChangesetsListViewColumn::Comment::GetToolTipText() { return LOCTEXT("Comment_Column_Tooltip", "Comment describing the changeset"); }

FName UnityVersionControlChangesetsListViewColumn::Branch::Id() { return TEXT("Branch"); }
FText UnityVersionControlChangesetsListViewColumn::Branch::GetDisplayText() { return LOCTEXT("Branch_Column", "Branch"); }
FText UnityVersionControlChangesetsListViewColumn::Branch::GetToolTipText() { return LOCTEXT("Branch_Column_Tooltip", "Branch where the changeset was created"); }

void SUnityVersionControlChangesetRow::Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwner)
{
	ChangesetToVisualize = InArgs._ChangesetToVisualize.Get();
	bIsCurrentChangeset = InArgs._bIsCurrentChangeset;
	HighlightText = InArgs._HighlightText;

	FSuperRowType::FArguments Args = FSuperRowType::FArguments()
		.ShowSelection(true);
	FSuperRowType::Construct(Args, InOwner);
}

TSharedRef<SWidget> SUnityVersionControlChangesetRow::GenerateWidgetForColumn(const FName& InColumnId)
{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
	const FSlateFontInfo FontInfo = bIsCurrentChangeset ? FAppStyle::GetFontStyle("BoldFont") : FAppStyle::GetFontStyle("NormalFont");
#else
	const FSlateFontInfo FontInfo = bIsCurrentChangeset ? FEditorStyle::GetFontStyle("BoldFont") : FEditorStyle::GetFontStyle("NormalFont");
#endif

	if (InColumnId == UnityVersionControlChangesetsListViewColumn::ChangesetId::Id())
	{
		return SNew(STextBlock)
			.Text(FText::AsNumber(ChangesetToVisualize->ChangesetId))
			.ToolTipText(FText::AsNumber(ChangesetToVisualize->ChangesetId))
			.Margin(FMargin(6.f, 1.f))
			.Font(FontInfo)
			.HighlightText(HighlightText);
	}
	else if (InColumnId == UnityVersionControlChangesetsListViewColumn::CreatedBy::Id())
	{
		return SNew(STextBlock)
			.Text(FText::FromString(UnityVersionControlUtils::UserNameToDisplayName(ChangesetToVisualize->CreatedBy)))
			.ToolTipText(FText::FromString(ChangesetToVisualize->CreatedBy))
			.Margin(FMargin(6.f, 1.f))
			.Font(FontInfo)
			.HighlightText(HighlightText);
	}
	else if (InColumnId == UnityVersionControlChangesetsListViewColumn::Date::Id())
	{
		return SNew(STextBlock)
			.Text(FText::AsDateTime(ChangesetToVisualize->Date))
			.ToolTipText(FText::AsDateTime(ChangesetToVisualize->Date))
			.Margin(FMargin(6.f, 1.f))
			.Font(FontInfo);
	}
	else if (InColumnId == UnityVersionControlChangesetsListViewColumn::Comment::Id())
	{
		// Make each comment fit on a single line to preserve the table layout
		FString CommentOnOneLine = ChangesetToVisualize->Comment;
		CommentOnOneLine.ReplaceCharInline(TEXT('\n'), TEXT(' '), ESearchCase::CaseSensitive);

		return SNew(STextBlock)
			.Text(FText::FromString(MoveTemp(CommentOnOneLine)))
			.ToolTipText(FText::FromString(ChangesetToVisualize->Comment))
			.Margin(FMargin(6.f, 1.f))
#if ENGINE_MAJOR_VERSION >= 5
			.OverflowPolicy(ETextOverflowPolicy::Ellipsis)
#endif
			.Font(FontInfo)
			.HighlightText(HighlightText);
	}
	else if (InColumnId == UnityVersionControlChangesetsListViewColumn::Branch::Id())
	{
		return SNew(STextBlock)
			.Text(FText::FromString(ChangesetToVisualize->Branch))
			.ToolTipText(FText::FromString(ChangesetToVisualize->Branch))
			.Margin(FMargin(6.f, 1.f))
#if ENGINE_MAJOR_VERSION >= 5
			.OverflowPolicy(ETextOverflowPolicy::Ellipsis)
#endif
			.Font(FontInfo)
			.HighlightText(HighlightText);
	}
	else
	{
		return SNullWidget::NullWidget;
	}
}

#undef LOCTEXT_NAMESPACE
