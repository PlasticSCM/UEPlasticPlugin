// Copyright (c) 2023 Unity Technologies

#include "UnityVersionControlWorkspaceCreation.h"

#include "UnityVersionControlOperations.h"
#include "UnityVersionControlModule.h"

#include "SourceControlOperations.h"
#include "ISourceControlModule.h"
#include "Misc/Paths.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"

#define LOCTEXT_NAMESPACE "FUnityVersionControlWorkspaceCreation"


void FUnityVersionControlWorkspaceCreation::MakeWorkspace(const FParameters& InParameters)
{
	WorkspaceParams = InParameters;

	// 1.a. Create a repository (if not already existing) and a workspace: launch an asynchronous MakeWorkspace operation
	LaunchMakeWorkspaceOperation();
}

/// 1. Create a repository (if not already existing) and a workspace
void FUnityVersionControlWorkspaceCreation::LaunchMakeWorkspaceOperation()
{
	TSharedRef<FPlasticMakeWorkspace, ESPMode::ThreadSafe> MakeWorkspaceOperation = ISourceControlOperation::Create<FPlasticMakeWorkspace>();
	MakeWorkspaceOperation->WorkspaceName = WorkspaceParams.WorkspaceName.ToString();
	MakeWorkspaceOperation->RepositoryName = WorkspaceParams.RepositoryName.ToString();
	MakeWorkspaceOperation->ServerUrl = WorkspaceParams.ServerUrl.ToString();
	MakeWorkspaceOperation->bPartialWorkspace = WorkspaceParams.bCreatePartialWorkspace;

	FUnityVersionControlProvider& Provider = FUnityVersionControlModule::Get().GetProvider();
	ECommandResult::Type Result = Provider.Execute(MakeWorkspaceOperation, TArray<FString>(), EConcurrency::Asynchronous, FSourceControlOperationComplete::CreateRaw(this, &FUnityVersionControlWorkspaceCreation::OnMakeWorkspaceOperationComplete));
	if (Result == ECommandResult::Succeeded)
	{
		DisplayInProgressNotification(MakeWorkspaceOperation->GetInProgressString());
	}
	else
	{
		DisplayFailureNotification(MakeWorkspaceOperation->GetName());
	}
}

void FUnityVersionControlWorkspaceCreation::OnMakeWorkspaceOperationComplete(const FSourceControlOperationRef& InOperation, ECommandResult::Type InResult)
{
	OnSourceControlOperationComplete(InOperation, InResult);

	// Launch the next asynchronous operation
	LaunchMarkForAddOperation();
}

/// 2. Add all project files to Source Control (.uproject, Config/, Content/, Source/ files and ignore.conf if any)
void FUnityVersionControlWorkspaceCreation::LaunchMarkForAddOperation()
{
	TSharedRef<FMarkForAdd, ESPMode::ThreadSafe> MarkForAddOperation = ISourceControlOperation::Create<FMarkForAdd>();
	FUnityVersionControlProvider& Provider = FUnityVersionControlModule::Get().GetProvider();

	// 1.b. Check the new workspace status to enable connection
	Provider.CheckPlasticAvailability();

	if (Provider.IsWorkspaceFound())
	{
		// 2. Add all project files to Source Control (.uproject, Config/, Content/, Source/ files and ignore.conf if any)
		const TArray<FString> ProjectFiles = GetProjectFiles();
		ECommandResult::Type Result = Provider.Execute(MarkForAddOperation, ProjectFiles, EConcurrency::Asynchronous, FSourceControlOperationComplete::CreateRaw(this, &FUnityVersionControlWorkspaceCreation::OnMarkForAddOperationComplete));
		if (Result == ECommandResult::Succeeded)
		{
			DisplayInProgressNotification(MarkForAddOperation->GetInProgressString());
		}
		else
		{
			DisplayFailureNotification(MarkForAddOperation->GetName());
		}
	}
	else
	{
		DisplayFailureNotification(MarkForAddOperation->GetName());
	}
}

void FUnityVersionControlWorkspaceCreation::OnMarkForAddOperationComplete(const FSourceControlOperationRef& InOperation, ECommandResult::Type InResult)
{
	OnSourceControlOperationComplete(InOperation, InResult);

	// Launch the next asynchronous operation
	LaunchCheckInOperation();
}

/// 3. Launch an asynchronous "CheckIn" operation and start another ongoing notification
void FUnityVersionControlWorkspaceCreation::LaunchCheckInOperation()
{
	TSharedRef<FCheckIn, ESPMode::ThreadSafe> CheckInOperation = ISourceControlOperation::Create<FCheckIn>();
	CheckInOperation->SetDescription(WorkspaceParams.InitialCommitMessage);
	FUnityVersionControlProvider& Provider = FUnityVersionControlModule::Get().GetProvider();
	const TArray<FString> ProjectFiles = GetProjectFiles(); // Note: listing files and folders is only needed for the update status operation following the checkin to know on what to operate
	ECommandResult::Type Result = Provider.Execute(CheckInOperation, ProjectFiles, EConcurrency::Asynchronous, FSourceControlOperationComplete::CreateRaw(this, &FUnityVersionControlWorkspaceCreation::OnCheckInOperationComplete));
	if (Result == ECommandResult::Succeeded)
	{
		DisplayInProgressNotification(CheckInOperation->GetInProgressString());
	}
	else
	{
		DisplayFailureNotification(CheckInOperation->GetName());
	}
}

void FUnityVersionControlWorkspaceCreation::OnCheckInOperationComplete(const FSourceControlOperationRef& InOperation, ECommandResult::Type InResult)
{
	OnSourceControlOperationComplete(InOperation, InResult);

	// Note: no more operation to launch, the workspace is ready to use
}

void FUnityVersionControlWorkspaceCreation::OnSourceControlOperationComplete(const FSourceControlOperationRef& InOperation, ECommandResult::Type InResult)
{
	RemoveInProgressNotification();

	// Report result with a notification
	if (InResult == ECommandResult::Succeeded)
	{
		DisplaySuccessNotification(InOperation->GetName());
	}
	else
	{
		DisplayFailureNotification(InOperation->GetName());
	}
}

// Display an ongoing notification during the whole operation
void FUnityVersionControlWorkspaceCreation::DisplayInProgressNotification(const FText& InOperationInProgressString)
{
	if (!OperationInProgressNotification.IsValid())
	{
		FNotificationInfo Info(InOperationInProgressString);
		Info.bFireAndForget = false;
		Info.ExpireDuration = 0.0f;
		Info.FadeOutDuration = 1.0f;
		OperationInProgressNotification = FSlateNotificationManager::Get().AddNotification(Info);
		if (OperationInProgressNotification.IsValid())
		{
			OperationInProgressNotification.Pin()->SetCompletionState(SNotificationItem::CS_Pending);
		}
	}
}

// Remove the ongoing notification at the end of the operation
void FUnityVersionControlWorkspaceCreation::RemoveInProgressNotification()
{
	if (OperationInProgressNotification.IsValid())
	{
		OperationInProgressNotification.Pin()->ExpireAndFadeout();
		OperationInProgressNotification.Reset();
	}
}

// Display a temporary success notification at the end of the operation
void FUnityVersionControlWorkspaceCreation::DisplaySuccessNotification(const FName& InOperationName)
{
	const FText NotificationText = FText::Format(LOCTEXT("InitWorkspace_Success", "{0} operation was successful!"), FText::FromName(InOperationName));
	FNotificationInfo Info(NotificationText);
	Info.bUseSuccessFailIcons = true;
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
	Info.Image = FAppStyle::GetBrush(TEXT("NotificationList.SuccessImage"));
#else
	Info.Image = FEditorStyle::GetBrush(TEXT("NotificationList.SuccessImage"));
#endif
	FSlateNotificationManager::Get().AddNotification(Info);
	UE_LOG(LogSourceControl, Verbose, TEXT("%s"), *NotificationText.ToString());
}

// Display a temporary failure notification at the end of the operation
void FUnityVersionControlWorkspaceCreation::DisplayFailureNotification(const FName& InOperationName)
{
	const FText NotificationText = FText::Format(LOCTEXT("InitWorkspace_Failure", "Error: {0} operation failed!"), FText::FromName(InOperationName));
	FNotificationInfo Info(NotificationText);
	Info.ExpireDuration = 8.0f;
	FSlateNotificationManager::Get().AddNotification(Info);
	UE_LOG(LogSourceControl, Error, TEXT("%s"), *NotificationText.ToString());
}

/** Path to the "ignore.conf" file */
const FString FUnityVersionControlWorkspaceCreation::GetIgnoreFileName() const
{
	const FString PathToWorkspaceRoot = FUnityVersionControlModule::Get().GetProvider().GetPathToWorkspaceRoot();
	const FString IgnoreFileName = FPaths::Combine(*PathToWorkspaceRoot, TEXT("ignore.conf"));
	return IgnoreFileName;
}

/** List of files to add to Source Control (.uproject, Config/, Content/, Source/ files and ignore.conf if any) */
TArray<FString> FUnityVersionControlWorkspaceCreation::GetProjectFiles() const
{
	TArray<FString> ProjectFiles;
	ProjectFiles.Add(FPaths::ConvertRelativePathToFull(FPaths::GetProjectFilePath()));
	ProjectFiles.Add(FPaths::ConvertRelativePathToFull(FPaths::ProjectConfigDir()));
	ProjectFiles.Add(FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir()));
	if (FPaths::DirectoryExists(FPaths::GameSourceDir()))
	{
		ProjectFiles.Add(FPaths::ConvertRelativePathToFull(FPaths::GameSourceDir()));
	}
	if (FPaths::FileExists(GetIgnoreFileName()))
	{
		ProjectFiles.Add(GetIgnoreFileName());
	}
	return ProjectFiles;
}

#undef LOCTEXT_NAMESPACE
