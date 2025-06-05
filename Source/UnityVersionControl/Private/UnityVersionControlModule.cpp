// Copyright (c) 2025 Unity Technologies

#include "UnityVersionControlModule.h"

#include "IUnityVersionControlWorker.h"

#include "Interfaces/IPluginManager.h"
#include "Features/IModularFeatures.h"
#include "Misc/App.h"
#include "Modules/ModuleManager.h"

#define LOCTEXT_NAMESPACE "UnityVersionControl"

void FUnityVersionControlModule::StartupModule()
{
	// Register our operations (implemented in UnityVersionControlOperations.cpp by sub-classing from Engine\Source\Developer\SourceControl\Public\SourceControlOperations.h)
	IUnityVersionControlWorker::RegisterWorkers(UnityVersionControlProvider);

	// Bind our source control provider to the editor
	IModularFeatures::Get().RegisterModularFeature("SourceControl", &UnityVersionControlProvider);

	/// Register our tab Window here as it needs to be ready for the editor to reload at startup
	UnityVersionControlBranchesWindow.Register();
	UnityVersionControlChangesetsWindow.Register();
	UnityVersionControlLocksWindow.Register();
}

void FUnityVersionControlModule::ShutdownModule()
{
	// shut down the provider, as this module is going away
	UnityVersionControlProvider.Close();

	UnityVersionControlBranchesWindow.Unregister();
	UnityVersionControlChangesetsWindow.Unregister();
	UnityVersionControlLocksWindow.Unregister();

	// unbind provider from editor
	IModularFeatures::Get().UnregisterModularFeature("SourceControl", &UnityVersionControlProvider);
}

FUnityVersionControlModule& FUnityVersionControlModule::Get()
{
	return FModuleManager::GetModuleChecked<FUnityVersionControlModule>("UnityVersionControl");
}

bool FUnityVersionControlModule::IsLoaded()
{
	return FModuleManager::Get().IsModuleLoaded("UnityVersionControl");
}

const TSharedPtr<IPlugin> FUnityVersionControlModule::GetPlugin()
{
	return IPluginManager::Get().FindPlugin(TEXT("UnityVersionControl"));;
}

IMPLEMENT_MODULE(FUnityVersionControlModule, UnityVersionControl);

#undef LOCTEXT_NAMESPACE
