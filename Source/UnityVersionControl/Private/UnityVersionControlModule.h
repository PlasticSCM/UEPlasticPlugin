// Copyright (c) 2025 Unity Technologies

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"

#include "UnityVersionControlProvider.h"
#include "UnityVersionControlWorkspaceCreation.h"

#include "UnityVersionControlBranchesWindow.h"
#include "UnityVersionControlChangesetsWindow.h"
#include "UnityVersionControlLocksWindow.h"

/**
 * UnityVersionControl is the official Unity Version Control Plugin for Unreal Engine
 *
 * Written and contributed by Sebastien Rombauts (sebastien.rombauts@gmail.com) for Codice Software
 */
class FUnityVersionControlModule : public IModuleInterface
{
public:
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	/** Access the Plastic source control provider */
	FUnityVersionControlProvider& GetProvider()
	{
		return UnityVersionControlProvider;
	}
	const FUnityVersionControlProvider& GetProvider() const
	{
		return UnityVersionControlProvider;
	}

	/** Access the controller to create a new workspace */
	FUnityVersionControlWorkspaceCreation& GetWorkspaceCreation()
	{
		return UnityVersionControlWorkspaceCreation;
	}

	FUnityVersionControlBranchesWindow& GetBranchesWindow()
	{
		return UnityVersionControlBranchesWindow;
	}

	FUnityVersionControlChangesetsWindow& GetChangesetsWindow()
	{
		return UnityVersionControlChangesetsWindow;
	}

	FUnityVersionControlLocksWindow& GetLocksWindow()
	{
		return UnityVersionControlLocksWindow;
	}

	/**
	 * Singleton-like access to this module's interface.  This is just for convenience!
	 * Beware of calling this during the shutdown phase, though.  Your module might have been unloaded already.
	 *
	 * @return Returns singleton instance, asserts if the module is not loaded yet or unloaded already.
	 */
	static FUnityVersionControlModule& Get();

	/**
	 * Checks whether the module is currently loaded.
	 */
	static bool IsLoaded();

	/**
	 * Finds information of the plugin.
	 *
	 * @return	 Pointer to the plugin's information, or nullptr.
	 */
	static const TSharedPtr<class IPlugin> GetPlugin();

private:
	/** The Plastic source control provider */
	FUnityVersionControlProvider UnityVersionControlProvider;

	/** Dockable windows adding advanced features to the plugin */
	FUnityVersionControlBranchesWindow UnityVersionControlBranchesWindow;
	FUnityVersionControlChangesetsWindow UnityVersionControlChangesetsWindow;
	FUnityVersionControlLocksWindow UnityVersionControlLocksWindow;

	/** Logic to create a new workspace */
	FUnityVersionControlWorkspaceCreation UnityVersionControlWorkspaceCreation;
};
