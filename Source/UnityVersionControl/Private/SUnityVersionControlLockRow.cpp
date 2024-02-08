// Copyright (c) 2024 Unity Technologies

#include "SUnityVersionControlLockRow.h"

#include "UnityVersionControlLock.h"
#include "UnityVersionControlUtils.h"

#include "Widgets/Text/STextBlock.h"

#include "Runtime/Launch/Resources/Version.h"
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
#include "Styling/AppStyle.h"
#else
#include "EditorStyleSet.h"
#endif

#define LOCTEXT_NAMESPACE "UnityVersionControlLockWindow"

FName UnityVersionControlLocksListViewColumn::ItemId::Id() { return TEXT("ItemId"); }
FText UnityVersionControlLocksListViewColumn::ItemId::GetDisplayText() { return LOCTEXT("Id_Column", "Item Id"); }
FText UnityVersionControlLocksListViewColumn::ItemId::GetToolTipText() { return LOCTEXT("Id_Column_Tooltip", "Displays the Id of the locked Item"); }

FName UnityVersionControlLocksListViewColumn::Path::Id() { return TEXT("Path"); }
FText UnityVersionControlLocksListViewColumn::Path::GetDisplayText() { return LOCTEXT("Path_Column", "Item"); }
FText UnityVersionControlLocksListViewColumn::Path::GetToolTipText() { return LOCTEXT("Path_Column_Tooltip", "Displays the item path"); }

FName UnityVersionControlLocksListViewColumn::Status::Id() { return TEXT("Status"); }
FText UnityVersionControlLocksListViewColumn::Status::GetDisplayText() { return LOCTEXT("Status_Column", "Status"); }
FText UnityVersionControlLocksListViewColumn::Status::GetToolTipText() { return LOCTEXT("Status_Column_Tooltip", "Displays the lock status"); }

FName UnityVersionControlLocksListViewColumn::Date::Id() { return TEXT("Date"); }
FText UnityVersionControlLocksListViewColumn::Date::GetDisplayText() { return LOCTEXT("Date_Column", "Modification date"); }
FText UnityVersionControlLocksListViewColumn::Date::GetToolTipText() { return LOCTEXT("Date_Column_Tooltip", "Displays the lock modification date"); }

FName UnityVersionControlLocksListViewColumn::Owner::Id() { return TEXT("Owner"); }
FText UnityVersionControlLocksListViewColumn::Owner::GetDisplayText() { return LOCTEXT("Owner_Column", "Owner"); }
FText UnityVersionControlLocksListViewColumn::Owner::GetToolTipText() { return LOCTEXT("Owner_Column_Tooltip", "Displays the name of the owner of the lock"); }

FName UnityVersionControlLocksListViewColumn::DestinationBranch::Id() { return TEXT("Destination Branch"); }
FText UnityVersionControlLocksListViewColumn::DestinationBranch::GetDisplayText() { return LOCTEXT("DestinationBranch_Column", "Destination Branch"); }
FText UnityVersionControlLocksListViewColumn::DestinationBranch::GetToolTipText() { return LOCTEXT("DestinationBranch_Column_Tooltip", "Displays the branch where the merge needs to happen in order to remove the lock"); }

FName UnityVersionControlLocksListViewColumn::Branch::Id() { return TEXT("Branch"); }
FText UnityVersionControlLocksListViewColumn::Branch::GetDisplayText() { return LOCTEXT("Branch_Column", "Branch"); }
FText UnityVersionControlLocksListViewColumn::Branch::GetToolTipText() { return LOCTEXT("Branch_Column_Tooltip", "Displays the branch where the lock has been created"); }

FName UnityVersionControlLocksListViewColumn::Workspace::Id() { return TEXT("Workspace"); }
FText UnityVersionControlLocksListViewColumn::Workspace::GetDisplayText() { return LOCTEXT("Workspace_Column", "Workspace"); }
FText UnityVersionControlLocksListViewColumn::Workspace::GetToolTipText() { return LOCTEXT("Workspace_Column_Tooltip", "Displays the workspace where the lock has been created"); }

void SUnityVersionControlLockRow::Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwner)
{
	LockToVisualize = InArgs._LockToVisualize.Get();
	HighlightText = InArgs._HighlightText;

	FSuperRowType::FArguments Args = FSuperRowType::FArguments()
		.ShowSelection(true);
	FSuperRowType::Construct(Args, InOwner);
}

TSharedRef<SWidget> SUnityVersionControlLockRow::GenerateWidgetForColumn(const FName& InColumnId)
{
	if (InColumnId == UnityVersionControlLocksListViewColumn::ItemId::Id())
	{
		return SNew(STextBlock)
			.Text(FText::AsNumber(LockToVisualize->ItemId))
			.ToolTipText(FText::AsNumber(LockToVisualize->ItemId))
			.Margin(FMargin(6.f, 1.f))
			.HighlightText(HighlightText);
	}
	else if (InColumnId == UnityVersionControlLocksListViewColumn::Path::Id())
	{
		return SNew(STextBlock)
			.Text(FText::FromString(LockToVisualize->Path))
			.ToolTipText(FText::FromString(LockToVisualize->Path))
			.Margin(FMargin(6.f, 1.f))
#if ENGINE_MAJOR_VERSION >= 5
			.OverflowPolicy(ETextOverflowPolicy::Ellipsis)
#endif
			.HighlightText(HighlightText);
	}
	else if (InColumnId == UnityVersionControlLocksListViewColumn::Status::Id())
	{
		return SNew(STextBlock)
			.Text(FText::FromString(LockToVisualize->Status))
			.ToolTipText(FText::FromString(LockToVisualize->Status))
			.Margin(FMargin(6.f, 1.f))
			.HighlightText(HighlightText);
	}
	else if (InColumnId == UnityVersionControlLocksListViewColumn::Date::Id())
	{
		return SNew(STextBlock)
			.Text(FText::AsDateTime(LockToVisualize->Date))
			.ToolTipText(FText::AsDateTime(LockToVisualize->Date))
			.Margin(FMargin(6.f, 1.f));
	}
	else if (InColumnId == UnityVersionControlLocksListViewColumn::Owner::Id())
	{
		return SNew(STextBlock)
			.Text(FText::FromString(UnityVersionControlUtils::UserNameToDisplayName(LockToVisualize->Owner)))
			.ToolTipText(FText::FromString(LockToVisualize->Owner))
			.Margin(FMargin(6.f, 1.f))
			.HighlightText(HighlightText);
	}
	else if (InColumnId == UnityVersionControlLocksListViewColumn::DestinationBranch::Id())
	{
		return SNew(STextBlock)
			.Text(FText::FromString(LockToVisualize->DestinationBranch))
			.ToolTipText(FText::FromString(LockToVisualize->DestinationBranch))
			.Margin(FMargin(6.f, 1.f))
			.HighlightText(HighlightText);
	}
	else if (InColumnId == UnityVersionControlLocksListViewColumn::Branch::Id())
	{
		return SNew(STextBlock)
			.Text(FText::FromString(LockToVisualize->Branch))
			.ToolTipText(FText::FromString(LockToVisualize->Branch))
			.Margin(FMargin(6.f, 1.f))
			.HighlightText(HighlightText);
	}
	else if (InColumnId == UnityVersionControlLocksListViewColumn::Workspace::Id())
	{
		return SNew(STextBlock)
			.Text(FText::FromString(LockToVisualize->Workspace))
			.ToolTipText(FText::FromString(LockToVisualize->Workspace))
			.Margin(FMargin(6.f, 1.f))
			.HighlightText(HighlightText);
	}
	else
	{
		return SNullWidget::NullWidget;
	}
}

#undef LOCTEXT_NAMESPACE
