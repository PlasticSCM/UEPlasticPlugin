// Copyright Unity Technologies

#pragma once

#include "CoreMinimal.h"

class FUnityVersionControlProvider;

class IUnityVersionControlWorker
{
public:
	// Implemented in UnityVersionControlOperations.cpp with all the code for the workers
	static void RegisterWorkers(FUnityVersionControlProvider& UnityVersionControlProvider);

public:
	explicit IUnityVersionControlWorker(FUnityVersionControlProvider& InSourceControlProvider)
		: UnityVersionControlProvider(InSourceControlProvider)
	{
	}

	virtual ~IUnityVersionControlWorker() = default;

	FUnityVersionControlProvider& GetProvider()
	{
		return UnityVersionControlProvider;
	}

	const FUnityVersionControlProvider& GetProvider() const
	{
		return UnityVersionControlProvider;
	}

	/**
	 * Name describing the work that this worker does. Used for factory method hookup.
	 */
	virtual FName GetName() const = 0;

	/**
	 * Function that actually does the work. Can be executed on another thread.
	 */
	virtual bool Execute(class FUnityVersionControlCommand& InCommand) = 0;

	/**
	 * Updates the state of any items after completion (if necessary). This is always executed on the main thread.
	 * @returns true if states were updated
	 */
	virtual bool UpdateStates() = 0;

private:
	FUnityVersionControlProvider& UnityVersionControlProvider;
};

typedef TSharedRef<IUnityVersionControlWorker, ESPMode::ThreadSafe> FUnityVersionControlWorkerRef;
