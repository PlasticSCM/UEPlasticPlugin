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
	TSharedRef<class SDockTab> OnSpawnPluginTab(const class FSpawnTabArgs& SpawnTabArgs);
};
