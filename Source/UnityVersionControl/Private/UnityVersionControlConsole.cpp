// Copyright (c) 2024 Unity Technologies

#include "UnityVersionControlConsole.h"

#include "UnityVersionControlUtils.h"

#include "ISourceControlModule.h"

void FUnityVersionControlConsole::Register()
{
	if (!CmConsoleCommand.IsValid())
	{
		CmConsoleCommand = MakeUnique<FAutoConsoleCommand>(
			TEXT("cm"),
			TEXT("Unity Version Control (formerly Plastic SCM) Command Line Interface (cm).\n")
			TEXT("Run any 'cm' command directly from the Unreal Editor Console.\n")
			TEXT("Type 'cm showcommands' to get a command list."),
			FConsoleCommandWithArgsDelegate::CreateRaw(this, &FUnityVersionControlConsole::ExecutePlasticConsoleCommand));
	}
}

void FUnityVersionControlConsole::Unregister()
{
	CmConsoleCommand.Reset();
}

void FUnityVersionControlConsole::ExecutePlasticConsoleCommand(const TArray<FString>& a_args)
{
	// If called with no argument, explicitly call "cm help" instead to mimic the cm CLI behavior.
	if (a_args.Num() < 1)
	{
		ExecutePlasticConsoleCommand(TArray<FString>({TEXT("help")}));
		return;
	}

	FString Results;
	FString Errors;
	const FString Command = a_args[0];
	TArray<FString> Parameters = a_args;
	Parameters.RemoveAt(0);
	UnityVersionControlUtils::RunCommand(Command, Parameters, TArray<FString>(), Results, Errors);
	if (Results.Len() > 200) // RunCommand() already log all command results up to 200 characters (limit to avoid long wall of text like XML)
	{
		UE_LOG(LogSourceControl, Log, TEXT("Output:\n%s"), *Results);
	}
}
