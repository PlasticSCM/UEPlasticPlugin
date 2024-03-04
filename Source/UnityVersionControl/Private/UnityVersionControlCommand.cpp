// Copyright (c) 2024 Unity Technologies

#include "UnityVersionControlCommand.h"

#include "UnityVersionControlModule.h"

#include "ISourceControlOperation.h"
#include "HAL/PlatformTime.h"

FUnityVersionControlCommand::FUnityVersionControlCommand(const FSourceControlOperationRef& InOperation, const FUnityVersionControlWorkerRef& InWorker, const FSourceControlOperationComplete& InOperationCompleteDelegate)
	: Operation(InOperation)
	, Worker(InWorker)
	, OperationCompleteDelegate(InOperationCompleteDelegate)
	, bExecuteProcessed(0)
	, bCommandSuccessful(false)
	, bConnectionDropped(false)
	, bAutoDelete(true)
	, Concurrency(EConcurrency::Synchronous)
	, StartTimestamp(FPlatformTime::Seconds())
{
	// grab the providers settings here, so we don't access them once the worker thread is launched
	check(IsInGameThread());
	const FUnityVersionControlProvider& Provider = FUnityVersionControlModule::Get().GetProvider();
	PathToWorkspaceRoot = Provider.GetPathToWorkspaceRoot();
	ChangesetNumber = Provider.GetChangesetNumber();
}

bool FUnityVersionControlCommand::DoWork()
{
	bCommandSuccessful = Worker->Execute(*this);
	FPlatformAtomics::InterlockedExchange(&bExecuteProcessed, 1);

	return bCommandSuccessful;
}

void FUnityVersionControlCommand::Abandon()
{
	FPlatformAtomics::InterlockedExchange(&bExecuteProcessed, 1);
}

void FUnityVersionControlCommand::DoThreadedWork()
{
	Concurrency = EConcurrency::Asynchronous;
	DoWork();
}

ECommandResult::Type FUnityVersionControlCommand::ReturnResults()
{
	// Save any messages that have accumulated
	for (FString& String : InfoMessages)
	{
		Operation->AddInfoMessge(FText::FromString(String));
	}
	for (FString& String : ErrorMessages)
	{
		Operation->AddErrorMessge(FText::FromString(String));
	}

	// run the completion delegate if we have one bound
	ECommandResult::Type Result = bCommandSuccessful ? ECommandResult::Succeeded : ECommandResult::Failed;
	OperationCompleteDelegate.ExecuteIfBound(Operation, Result);

	return Result;
}
