// Copyright Unity Technologies

#include "UnityVersionControlModule.h"

#include "IUnityVersionControlWorker.h"
#include "Features/IModularFeatures.h"
#include "Misc/App.h"

#define LOCTEXT_NAMESPACE "UnityVersionControl"

void FUnityVersionControlModule::StartupModule()
{
	// Register our operations (implemented in UnityVersionControlOperations.cpp by sub-classing from Engine\Source\Developer\SourceControl\Public\SourceControlOperations.h)
	IUnityVersionControlWorker::RegisterWorkers(UnityVersionControlProvider);

	// Bind our source control provider to the editor
	IModularFeatures::Get().RegisterModularFeature("SourceControl", &UnityVersionControlProvider);
}

void FUnityVersionControlModule::ShutdownModule()
{
	// shut down the provider, as this module is going away
	UnityVersionControlProvider.Close();

	// unbind provider from editor
	IModularFeatures::Get().UnregisterModularFeature("SourceControl", &UnityVersionControlProvider);
}

IMPLEMENT_MODULE(FUnityVersionControlModule, UnityVersionControl);

#undef LOCTEXT_NAMESPACE
