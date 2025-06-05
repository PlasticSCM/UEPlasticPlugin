// Copyright (c) 2025 Unity Technologies

#include "SUnityVersionControlBranchRow.h"

#include "UnityVersionControlBranch.h"
#include "UnityVersionControlUtils.h"

#include "Widgets/Text/STextBlock.h"

#include "Runtime/Launch/Resources/Version.h"
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
#include "Styling/AppStyle.h"
#else
#include "EditorStyleSet.h"
#endif

#define LOCTEXT_NAMESPACE "UnityVersionControlBranchesWindow"

FName UnityVersionControlBranchesListViewColumn::Name::Id() { return TEXT("Name"); }
FText UnityVersionControlBranchesListViewColumn::Name::GetDisplayText() { return LOCTEXT("Name_Column", "Name"); }
FText UnityVersionControlBranchesListViewColumn::Name::GetToolTipText() { return LOCTEXT("Name_Column_Tooltip", "Displays the asset/file name"); }

FName UnityVersionControlBranchesListViewColumn::Repository::Id() { return TEXT("Repository"); }
FText UnityVersionControlBranchesListViewColumn::Repository::GetDisplayText() { return LOCTEXT("Repository_Column", "Repository"); }
FText UnityVersionControlBranchesListViewColumn::Repository::GetToolTipText() { return LOCTEXT("Repository_Column_Tooltip", "Displays the repository where the branch has been created"); }

FName UnityVersionControlBranchesListViewColumn::CreatedBy::Id() { return TEXT("CreatedBy"); }
FText UnityVersionControlBranchesListViewColumn::CreatedBy::GetDisplayText() { return LOCTEXT("CreatedBy_Column", "Created by"); }
FText UnityVersionControlBranchesListViewColumn::CreatedBy::GetToolTipText() { return LOCTEXT("CreatedBy_Column_Tooltip", "Displays the name of the creator of the branch"); }

FName UnityVersionControlBranchesListViewColumn::Date::Id() { return TEXT("Date"); }
FText UnityVersionControlBranchesListViewColumn::Date::GetDisplayText() { return LOCTEXT("Date_Column", "Creation date"); }
FText UnityVersionControlBranchesListViewColumn::Date::GetToolTipText() { return LOCTEXT("Date_Column_Tooltip", "Displays the branch creation date"); }

FName UnityVersionControlBranchesListViewColumn::Comment::Id() { return TEXT("Comment"); }
FText UnityVersionControlBranchesListViewColumn::Comment::GetDisplayText() { return LOCTEXT("Comment_Column", "Comment"); }
FText UnityVersionControlBranchesListViewColumn::Comment::GetToolTipText() { return LOCTEXT("Comment_Column_Tooltip", "Displays the branch comment"); }

void SUnityVersionControlBranchRow::Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwner)
{
	BranchToVisualize = InArgs._BranchToVisualize.Get();
	bIsCurrentBranch = InArgs._bIsCurrentBranch;
	HighlightText = InArgs._HighlightText;

	FSuperRowType::FArguments Args = FSuperRowType::FArguments()
		.ShowSelection(true);
	FSuperRowType::Construct(Args, InOwner);
}

TSharedRef<SWidget> SUnityVersionControlBranchRow::GenerateWidgetForColumn(const FName& InColumnId)
{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
	const FSlateFontInfo FontInfo = bIsCurrentBranch ? FAppStyle::GetFontStyle("BoldFont") : FAppStyle::GetFontStyle("NormalFont");
#else
	const FSlateFontInfo FontInfo = bIsCurrentBranch ? FEditorStyle::GetFontStyle("BoldFont") : FEditorStyle::GetFontStyle("NormalFont");
#endif

	if (InColumnId == UnityVersionControlBranchesListViewColumn::Name::Id())
	{
		return SNew(STextBlock)
			.Text(FText::FromString(BranchToVisualize->Name))
			.ToolTipText(FText::FromString(BranchToVisualize->Name))
			.Margin(FMargin(6.f, 1.f))
#if ENGINE_MAJOR_VERSION >= 5
			.OverflowPolicy(ETextOverflowPolicy::Ellipsis)
#endif
			.Font(FontInfo)
			.HighlightText(HighlightText);
	}
	else if (InColumnId == UnityVersionControlBranchesListViewColumn::Repository::Id())
	{
		return SNew(STextBlock)
			.Text(FText::FromString(BranchToVisualize->Repository))
			.ToolTipText(FText::FromString(BranchToVisualize->Repository))
			.Margin(FMargin(6.f, 1.f))
			.Font(FontInfo)
			.HighlightText(HighlightText);
	}
	else if (InColumnId == UnityVersionControlBranchesListViewColumn::CreatedBy::Id())
	{
		return SNew(STextBlock)
			.Text(FText::FromString(UnityVersionControlUtils::UserNameToDisplayName(BranchToVisualize->CreatedBy)))
			.ToolTipText(FText::FromString(BranchToVisualize->CreatedBy))
			.Margin(FMargin(6.f, 1.f))
			.Font(FontInfo)
			.HighlightText(HighlightText);
	}
	else if (InColumnId == UnityVersionControlBranchesListViewColumn::Date::Id())
	{
		return SNew(STextBlock)
			.Text(FText::AsDateTime(BranchToVisualize->Date))
			.ToolTipText(FText::AsDateTime(BranchToVisualize->Date))
			.Margin(FMargin(6.f, 1.f))
			.Font(FontInfo);
	}
	else if (InColumnId == UnityVersionControlBranchesListViewColumn::Comment::Id())
	{
		// Make each comment fit on a single line to preserve the table layout
		FString CommentOnOneLine = BranchToVisualize->Comment;
		CommentOnOneLine.ReplaceCharInline(TEXT('\n'), TEXT(' '), ESearchCase::CaseSensitive);

		return SNew(STextBlock)
			.Text(FText::FromString(MoveTemp(CommentOnOneLine)))
			.ToolTipText(FText::FromString(BranchToVisualize->Comment))
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
