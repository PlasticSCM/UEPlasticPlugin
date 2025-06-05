// Copyright (c) 2025 Unity Technologies

#pragma once

#include "CoreMinimal.h"

#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Views/STableRow.h"
#include "Widgets/Views/STableViewBase.h"

typedef TSharedRef<class FUnityVersionControlState, ESPMode::ThreadSafe> FUnityVersionControlStateRef;
typedef TSharedPtr<class FUnityVersionControlState, ESPMode::ThreadSafe> FUnityVersionControlStatePtr;

class FUnityVersionControlState;

/** Lists the unique columns used in the list view displaying Files in the selected changeset. */
namespace UnityVersionControlChangesetFilesListViewColumn
{
	/** The Icon displaying the type of Change column. */
	namespace Icon // NOLINT(runtime/indentation_namespace)
	{
		FName Id();
		FText GetDisplayText();
		FText GetToolTipText();
	};

	/** The File Name column. */
	namespace Name // NOLINT(runtime/indentation_namespace)
	{
		FName Id();
		FText GetDisplayText();
		FText GetToolTipText();
	};

	/** The File Path column. */
	namespace Path // NOLINT(runtime/indentation_namespace)
	{
		FName Id();
		FText GetDisplayText();
		FText GetToolTipText();
	};

} // namespace UnityVersionControlChangesetFilesListViewColumn

class SUnityVersionControlChangesetFileRow : public SMultiColumnTableRow<FUnityVersionControlStateRef>
{
public:
	SLATE_BEGIN_ARGS(SUnityVersionControlChangesetFileRow)
		: _FileToVisualize(nullptr)
		, _HighlightText()
	{
	}
		SLATE_ARGUMENT(FUnityVersionControlStatePtr, FileToVisualize)
		SLATE_ATTRIBUTE(FText, HighlightText)
	SLATE_END_ARGS()

public:
	/**
	* Construct a row child widgets of the ListView.
	*
	* @param InArgs Parameters including the File to visualize in this row.
	* @param InOwner The owning ListView.
	*/
	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwner);

	// SMultiColumnTableRow overrides
	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& ColumnId) override;

private:
	/** The File that we are visualizing in this row. */
	FUnityVersionControlState* FileToVisualize;

	/** The search text to highlight if any */
	TAttribute<FText> HighlightText;
};
