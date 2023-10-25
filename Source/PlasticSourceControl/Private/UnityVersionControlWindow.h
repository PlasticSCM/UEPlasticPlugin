// Copyright (c) 2023 Unity Technologies

#pragma once

#include "CoreMinimal.h"


// TODO POC
#include "UnityVersionControlWindow.generated.h"




class FUnityVersionControlWindow
{
public:
	void Register();
	void Unregister();
	
	void OpenTab();

private:
	TSharedRef<class SDockTab> OnSpawnTab(const class FSpawnTabArgs& SpawnTabArgs);

	// TODO POC
	TSharedPtr<SWidget> CreateCacheBranchesWidget();
};


// TODO POC

/** Context object for SSourceControlHistoryWidget's right-click context menu to get info about which history widget is trying to generate the menu */
UCLASS()
class PLASTICSOURCECONTROL_API UBranchesWidgetContext : public UObject
{
	GENERATED_BODY()

public:
	TWeakPtr<class SBranchesWidget> BranchesWidget;

	FString& GetSelectedBranch()
	{
		return SelectedBranch;
	}

private:
	FString SelectedBranch;
};


// TODO POC
// NOTE: inspired from SUnsavedAssetsStatusBarWidget
class SBranchesWidget : public SCompoundWidget
{
	SLATE_BEGIN_ARGS(SBranchesWidget) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:

	TSharedRef<SWidget> GetGridPanel();

	EActiveTimerReturnType UpdateGridPanels(double InCurrentTime, float InDeltaTime);

	SVerticalBox::FSlot* GridSlot = nullptr;

	FString FilterText;

	void OnFilterTextChanged(const FText& SearchText);

	static void CreatePOCMenu(UToolMenu* InToolMenu);
	void OnPOCMenuAction();
	TSharedPtr<SWidget> OnCreateContextMenu();

	/** Delegate called when the source control window is closed */
	void OnCreateBranchDialogClosed(const TSharedRef<class SWindow>& InWindow);

	/** The create branch window we may be using */
	TSharedPtr<SWindow> CreateBranchWindowPtr;

	/** The create branch window control we may be using */
	TSharedPtr<class SCreateBranch> CreateBranchContentPtr;
};

// TODO POC move this to a separate file, with a different name etc!

class SCreateBranch : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SCreateBranch)
		: _ParentWindow()
		, _BranchName()
	{}

		SLATE_ATTRIBUTE(TSharedPtr<SWindow>, ParentWindow)
		SLATE_ATTRIBUTE(FString, BranchName)

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	void OnCheckedSwitchWorkspace(ECheckBoxState InCheckedState);

	/** Called when the settings of the dialog are to be accepted */
	FReply CreateClicked();

	/** Called when the settings of the dialog are to be ignored */
	FReply CancelClicked();

	TSharedPtr<class SEditableTextBox> BranchNameTextCtrl;
	TSharedPtr<class SMultiLineEditableTextBox> BranchCommentsTextCtrl;
	TSharedPtr<class SCheckBox> SwitchWorkspaceCheckBoxCtrl;

private:

	/** Pointer to the parent modal window */
	TWeakPtr<SWindow> ParentWindow;
	FString BranchName;

	bool bSwitchWorkspace = true;

};

