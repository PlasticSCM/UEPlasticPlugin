// Copyright (c) 2023 Unity Technologies

#pragma once

#include "CoreMinimal.h"

#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"

class FReply;
class SEditableTextBox;
class SUnityVersionControlBranchesWidget;
class SWindow;
enum class ECheckBoxState : uint8;

class SUnityVersionControlCreateBranch : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SUnityVersionControlCreateBranch)
		: _BranchesWidget()
		, _ParentWindow()
		, _ParentBranchName()
	{}

		SLATE_ARGUMENT(TSharedPtr<SUnityVersionControlBranchesWidget>, BranchesWidget)
		SLATE_ARGUMENT(TSharedPtr<SWindow>, ParentWindow)
		SLATE_ARGUMENT(FString, ParentBranchName)

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	void OnCheckedSwitchWorkspace(ECheckBoxState InState);

	bool CanCreateBranch() const;
	FText CreateButtonTooltip() const;

	FReply CreateClicked();
	FReply CancelClicked();

	/** Interpret Escape as Cancel */
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;

private:
	FString ParentBranchName;
	FString NewBranchName;
	FString NewBranchComment;
	bool bSwitchWorkspace = true;

	TSharedPtr<SEditableTextBox> BranchNameTextBox;

	TWeakPtr<SUnityVersionControlBranchesWidget> BranchesWidget;
	TWeakPtr<SWindow> ParentWindow;
};
