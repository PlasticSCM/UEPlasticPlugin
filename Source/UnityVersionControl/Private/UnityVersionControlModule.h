// Copyright Unity Technologies

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"
#include "UnityVersionControlProvider.h"

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

	/**
	 * Singleton-like access to this module's interface.  This is just for convenience!
	 * Beware of calling this during the shutdown phase, though.  Your module might have been unloaded already.
	 *
	 * @return Returns singleton instance, loading the module on demand if needed
	 */
	static inline FUnityVersionControlModule& Get()
	{
		return FModuleManager::GetModuleChecked<FUnityVersionControlModule>("UnityVersionControl");
	}

private:
	/** The Plastic source control provider */
	FUnityVersionControlProvider UnityVersionControlProvider;
};
