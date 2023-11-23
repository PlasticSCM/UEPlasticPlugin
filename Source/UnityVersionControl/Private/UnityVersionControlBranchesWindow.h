// Copyright (c) 2023 Unity Technologies

#pragma once

#include "CoreMinimal.h"

// Nomad tab window to hold the widget with the list of branches, see SUnityVersionControlBranchesWidget
class FUnityVersionControlBranchesWindow
{
public:
	void Register();
	void Unregister();

	void OpenTab();

private:
	TSharedRef<class SDockTab> OnSpawnTab(const class FSpawnTabArgs& SpawnTabArgs);

	TSharedPtr<SWidget> CreateBranchesWidget();
};
