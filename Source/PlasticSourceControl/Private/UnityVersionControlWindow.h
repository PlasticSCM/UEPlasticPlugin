// Copyright (c) 2023 Unity Technologies

#pragma once

#include "CoreMinimal.h"

class FUnityVersionControlWindow
{
public:
	void Register();
	void Unregister();
	
	void OpenTab();

private:
	TSharedRef<class SDockTab> OnSpawnTab(const class FSpawnTabArgs& SpawnTabArgs);

	// TODO POC
	TSharedPtr<SWidget> CreateCacheStatisticsDialog();
};

// TODO POC
class SDerivedDataCacheStatisticsDialog : public SCompoundWidget
{
	SLATE_BEGIN_ARGS(SDerivedDataCacheStatisticsDialog) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:

	TSharedRef<SWidget> GetGridPanel();

	EActiveTimerReturnType UpdateGridPanels(double InCurrentTime, float InDeltaTime);

	SVerticalBox::FSlot* GridSlot = nullptr;

	FString FilterText;

	void OnFilterTextChanged(const FText& SearchText);
};
