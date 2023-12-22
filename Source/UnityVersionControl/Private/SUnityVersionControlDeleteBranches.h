// Copyright (c) 2023 Unity Technologies

#pragma once

#include "CoreMinimal.h"

#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"

class FReply;
class SButton;
class SUnityVersionControlBranchesWidget;
class SWindow;

class SUnityVersionControlDeleteBranches : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SUnityVersionControlDeleteBranches)
		: _BranchesWidget()
		, _ParentWindow()
		, _BranchNames()
	{}

		SLATE_ARGUMENT(TSharedPtr<SUnityVersionControlBranchesWidget>, BranchesWidget)
		SLATE_ARGUMENT(TSharedPtr<SWindow>, ParentWindow)
		SLATE_ARGUMENT(TArray<FString>, BranchNames)

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	FReply DeleteClicked();
	FReply CancelClicked();

	/** Interpret Escape as Cancel */
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;

private:
	TArray<FString> BranchNames;

	TWeakPtr<SUnityVersionControlBranchesWidget> BranchesWidget;
	TWeakPtr<SWindow> ParentWindow;

	TSharedPtr<SButton> DeleteButtonPtr;
};
