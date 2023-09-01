// Copyright (c) 2023 Unity Technologies

#include "SUnityVersionControlSettings.h"
#include "UnityVersionControlOperations.h"
#include "UnityVersionControlModule.h"
#include "UnityVersionControlUtils.h"

#include "SourceControlOperations.h"
#include "ISourceControlModule.h"
#include "Fonts/SlateFontInfo.h"
#include "Misc/Paths.h"
#include "Misc/App.h"
#include "Misc/FileHelper.h"
#include "Modules/ModuleManager.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Styling/SlateTypes.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Runtime/Launch/Resources/Version.h"
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
#include "Styling/AppStyle.h"
#else
#include "EditorStyleSet.h"
#endif

#define LOCTEXT_NAMESPACE "SUnityVersionControlSettings"

void SUnityVersionControlSettings::Construct(const FArguments& InArgs)
{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
	const FSlateFontInfo Font = FAppStyle::GetFontStyle(TEXT("SourceControl.LoginWindow.Font"));
#else
	const FSlateFontInfo Font = FEditorStyle::GetFontStyle(TEXT("SourceControl.LoginWindow.Font"));
#endif

	bAutoCreateIgnoreFile = CanAutoCreateIgnoreFile();
	bAutoInitialCommit = true;

	InitialCommitMessage = LOCTEXT("InitialCommitMessage", "Initial checkin");
	ServerUrl = FText::FromString(TEXT("YourOrganization@cloud"));
	if (FApp::HasProjectName())
	{
		WorkspaceName = FText::FromString(FApp::GetProjectName());
		RepositoryName = WorkspaceName;
	}

	ChildSlot
	[
#if ENGINE_MAJOR_VERSION == 4
	SNew(SBorder)
	.BorderImage(FEditorStyle::GetBrush("DetailsView.CategoryBottom"))
	.Padding(FMargin(0.0f, 3.0f, 0.0f, 0.0f))
	[
#endif
		SNew(SVerticalBox)
		// Versions (Plugin & Unity Version Control) useful eg to help diagnose issues from screenshots
		+SVerticalBox::Slot()
		.AutoHeight()
		.Padding(2.0f)
		.VAlign(VAlign_Center)
		[
			SNew(SHorizontalBox)
			.ToolTipText(LOCTEXT("PlasticVersions_Tooltip", "Unity Version Control (formerly Plastic SCM) and Plugin versions"))
			+SHorizontalBox::Slot()
			.FillWidth(1.0f)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("PlasticVersions", "Unity Version Control"))
				.Font(Font)
			]
			+SHorizontalBox::Slot()
			.FillWidth(2.0f)
			[
				SNew(STextBlock)
				.Text(this, &SUnityVersionControlSettings::GetVersions)
				.Font(Font)
			]
		]
		// Unity Version Control command line tool not available warning
		+SVerticalBox::Slot()
		.FillHeight(1.0f)
		.Padding(2.0f)
		[
			SNew(STextBlock)
			.Visibility(this, &SUnityVersionControlSettings::PlasticNotAvailable)
			.ToolTipText(LOCTEXT("PlasticNotAvailable_Tooltip", "Failed to launch Unity Version Control 'cm' command line tool. You need to install it and make sure that 'cm' is on the Path and correctly configured."))
			.Text(LOCTEXT("PlasticNotAvailable", "Unity Version Control Command Line tool 'cm' failed to start:"))
			.Font(Font)
		]
		// Path to the Unity Version Control binary
		+SVerticalBox::Slot()
		.AutoHeight()
		.Padding(2.0f)
		.VAlign(VAlign_Center)
		[
			SNew(SHorizontalBox)
			.ToolTipText(LOCTEXT("BinaryPathLabel_Tooltip", "Path to the Unity Version Control Command Line tool 'cm' executable"))
			+SHorizontalBox::Slot()
			.FillWidth(1.0f)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("PathLabel", "Path to cm"))
				.Font(Font)
			]
			+SHorizontalBox::Slot()
			.FillWidth(2.0f)
			[
				SNew(SEditableTextBox)
				.Text(this, &SUnityVersionControlSettings::GetBinaryPathText)
				.HintText(LOCTEXT("BinaryPathLabel", "Path to the Unity Version Control 'cm' executable"))
				.OnTextCommitted(this, &SUnityVersionControlSettings::OnBinaryPathTextCommited)
				.Font(Font)
			]
		]
		// Root of the workspace
		+SVerticalBox::Slot()
		.AutoHeight()
		.Padding(2.0f)
		.VAlign(VAlign_Center)
		[
			SNew(SHorizontalBox)
			.ToolTipText(this, &SUnityVersionControlSettings::GetPathToWorkspaceRoot)
			+SHorizontalBox::Slot()
			.FillWidth(1.0f)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("WorkspaceRootLabel", "Root of the workspace"))
				.Font(Font)
			]
			+SHorizontalBox::Slot()
			.FillWidth(2.0f)
			[
				SNew(STextBlock)
				.Text(this, &SUnityVersionControlSettings::GetPathToWorkspaceRoot)
				.Font(Font)
			]
		]
		// User Name
		+SVerticalBox::Slot()
		.AutoHeight()
		.Padding(2.0f)
		.VAlign(VAlign_Center)
		[
			SNew(SHorizontalBox)
			.ToolTipText(LOCTEXT("PlasticUserName_Tooltip", "User name configured for the Unity Version Control workspace"))
			+SHorizontalBox::Slot()
			.FillWidth(1.0f)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("PlasticUserName", "User Name"))
				.Font(Font)
			]
			+SHorizontalBox::Slot()
			.FillWidth(2.0f)
			[
				SNew(STextBlock)
				.Text(this, &SUnityVersionControlSettings::GetUserName)
				.Font(Font)
			]
		]
		// Separator
		+SVerticalBox::Slot()
		.AutoHeight()
		.Padding(2.0f)
		.VAlign(VAlign_Center)
		[
			SNew(SSeparator)
		]
		// Explanation text
		+SVerticalBox::Slot()
		.FillHeight(1.0f)
		.Padding(2.0f, 5.0f)
		[
			SNew(STextBlock)
			.Visibility(this, &SUnityVersionControlSettings::CanCreatePlasticWorkspace)
			.ToolTipText(LOCTEXT("WorkspaceNotFound_Tooltip", "No Workspace found at the level or above the current Project. Use the form to create a new one."))
			.Text(LOCTEXT("WorkspaceNotFound", "Current Project is not in a Unity Version Control Workspace. Create a new one:"))
			.Font(Font)
		]
		// Workspace and Repository Name
		+SVerticalBox::Slot()
		.AutoHeight()
		.Padding(2.0f)
		.VAlign(VAlign_Center)
		[
			SNew(SHorizontalBox)
			.Visibility(this, &SUnityVersionControlSettings::CanCreatePlasticWorkspace)
			+SHorizontalBox::Slot()
			.FillWidth(1.0f)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("WorkspaceRepositoryName", "Workspace & Repository"))
				.ToolTipText(LOCTEXT("WorkspaceRepositoryName_Tooltip", "Enter the Name of the new Workspace and Repository to create or use"))
				.Font(Font)
			]
			+SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.Padding(2.0f, 0.0f, 0.0f, 0.0f)
			[
				SNew(SEditableTextBox)
				.Text(this, &SUnityVersionControlSettings::GetWorkspaceName)
				.ToolTipText(LOCTEXT("WorkspaceName_Tooltip", "Enter the Name of the new Workspace to create"))
				.HintText(LOCTEXT("WorkspaceName_Hint", "Name of the Workspace to create"))
				.OnTextCommitted(this, &SUnityVersionControlSettings::OnWorkspaceNameCommited)
				.Font(Font)
			]
			+SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.Padding(2.0f, 0.0f, 0.0f, 0.0f)
			[
				SNew(SEditableTextBox)
				.Text(this, &SUnityVersionControlSettings::GetRepositoryName)
				.ToolTipText(LOCTEXT("RepositoryName_Tooltip", "Enter the Name of the Repository to use or create"))
				.HintText(LOCTEXT("RepositoryName_Hint", "Name of the Repository to use or create"))
				.OnTextCommitted(this, &SUnityVersionControlSettings::OnRepositoryNameCommited)
				.Font(Font)
			]
		]
		// Server URL address:port
		+SVerticalBox::Slot()
		.AutoHeight()
		.Padding(2.0f)
		.VAlign(VAlign_Center)
		[
			SNew(SHorizontalBox)
			.Visibility(this, &SUnityVersionControlSettings::CanCreatePlasticWorkspace)
			.ToolTipText(LOCTEXT("ServerUrl_Tooltip", "Enter the Server URL in the form address:port (eg. YourOrganization@cloud, local, or something like ip:port, eg localhost:8087)"))
			+SHorizontalBox::Slot()
			.FillWidth(1.0f)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("ServerUrl", "Server URL address:port"))
				.Font(Font)
			]
			+SHorizontalBox::Slot()
			.FillWidth(2.0f)
			[
				SNew(SEditableTextBox)
				.Text(this, &SUnityVersionControlSettings::GetServerUrl)
				.HintText(LOCTEXT("EnterServerUrl", "Enter the Server URL"))
				.OnTextCommitted(this, &SUnityVersionControlSettings::OnServerUrlCommited)
				.Font(Font)
			]
		]
		// Option to create a Partial/Gluon Workspace designed
		+SVerticalBox::Slot()
		.AutoHeight()
		.Padding(2.0f)
		.VAlign(VAlign_Center)
		[
			SNew(SCheckBox)
			.Visibility(this, &SUnityVersionControlSettings::CanCreatePlasticWorkspace)
			.ToolTipText(LOCTEXT("CreatePartialWorkspace_Tooltip", "Create the new workspace in Gluon/partial mode, designed for artists."))
			.IsChecked(bCreatePartialWorkspace)
			.OnCheckStateChanged(this, &SUnityVersionControlSettings::OnCheckedCreatePartialWorkspace)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("CreatePartialWorkspace", "Make the new workspace a Gluon partial workspace."))
				.Font(Font)
			]
		]
		// Option to add a 'ignore.conf' file at Workspace creation time
		+SVerticalBox::Slot()
		.AutoHeight()
		.Padding(2.0f)
		.VAlign(VAlign_Center)
		[
			SNew(SCheckBox)
			.Visibility(this, &SUnityVersionControlSettings::CanCreatePlasticWorkspace)
			.ToolTipText(LOCTEXT("CreateIgnoreFile_Tooltip", "Create and add a standard 'ignore.conf' file"))
			.IsEnabled(this, &SUnityVersionControlSettings::CanAutoCreateIgnoreFile)
			.IsChecked(bAutoCreateIgnoreFile)
			.OnCheckStateChanged(this, &SUnityVersionControlSettings::OnCheckedCreateIgnoreFile)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("CreateIgnoreFile", "Add a ignore.conf file"))
				.Font(Font)
			]
		]
		// Option to Make the initial Unity Version Control checkin
		+SVerticalBox::Slot()
		.AutoHeight()
		.Padding(2.0f)
		.VAlign(VAlign_Center)
		[
			SNew(SHorizontalBox)
			.Visibility(this, &SUnityVersionControlSettings::CanCreatePlasticWorkspace)
			.ToolTipText(LOCTEXT("InitialCommit_Tooltip", "Make the initial Unity Version Control checkin"))
			+SHorizontalBox::Slot()
			.FillWidth(0.7f)
			[
				SNew(SCheckBox)
				.IsChecked(ECheckBoxState::Checked)
				.OnCheckStateChanged(this, &SUnityVersionControlSettings::OnCheckedInitialCommit)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("InitialCommit", "Initial Checkin"))
					.Font(Font)
				]
			]
			+SHorizontalBox::Slot()
			.FillWidth(1.4f)
			.Padding(2.0f)
			[
				SNew(SMultiLineEditableTextBox)
				.Text(this, &SUnityVersionControlSettings::GetInitialCommitMessage)
				.HintText(LOCTEXT("InitialCommitMessage_Hint", "Message for the initial checkin"))
				.OnTextCommitted(this, &SUnityVersionControlSettings::OnInitialCommitMessageCommited)
				.Font(Font)
			]
		]
		// Option to run an Update Status operation at Editor Startup
		+SVerticalBox::Slot()
		.AutoHeight()
		.Padding(2.0f)
		.VAlign(VAlign_Center)
		[
			SNew(SCheckBox)
			.IsChecked(SUnityVersionControlSettings::IsUpdateStatusAtStartupChecked())
			.OnCheckStateChanged(this, &SUnityVersionControlSettings::OnCheckedUpdateStatusAtStartup)
			.ToolTipText(LOCTEXT("UpdateStatusAtStartup_Tooltip", "Run an asynchronous Update Status at Editor startup (can be slow)."))
			[
				SNew(STextBlock)
				.Text(LOCTEXT("UpdateStatusAtStartup", "Update workspace Status at Editor startup"))
				.Font(Font)
			]
		]
		// Option to call History as part of Update Status operation to check for potential recent changes in other branches
		+SVerticalBox::Slot()
		.AutoHeight()
		.Padding(2.0f)
		.VAlign(VAlign_Center)
		[
			SNew(SCheckBox)
			.ToolTipText(LOCTEXT("UpdateStatusOtherBranches_Tooltip", "Enable Update status to detect more recent changes on other branches in order to display warnings (can be slow)."))
			.IsChecked(SUnityVersionControlSettings::IsUpdateStatusOtherBranchesChecked())
			.OnCheckStateChanged(this, &SUnityVersionControlSettings::OnCheckedUpdateStatusOtherBranches)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("UpdateStatusOtherBranches", "Update Status also checks history to detect changes on other branches."))
				.Font(Font)
			]
		]
		// Option to enable Source Control Verbose logs
		+SVerticalBox::Slot()
		.AutoHeight()
		.Padding(2.0f)
		.VAlign(VAlign_Center)
		[
			SNew(SCheckBox)
			.ToolTipText(LOCTEXT("EnableVerboseLogs_Tooltip", "Override LogSourceControl default verbosity level to Verbose (except if already set to VeryVerbose)."))
			.IsChecked(SUnityVersionControlSettings::IsEnableVerboseLogsChecked())
			.OnCheckStateChanged(this, &SUnityVersionControlSettings::OnCheckedEnableVerboseLogs)
			[
				SNew(STextBlock)
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 2
				.Text(LOCTEXT("EnableVerboseLogs", "Enable Revision Control Verbose logs"))
#else
				.Text(LOCTEXT("EnableVerboseLogs", "Enable Source Control Verbose logs"))
#endif
				.Font(Font)
			]
		]
		// Button to create a new Workspace
		+SVerticalBox::Slot()
		.FillHeight(2.5f)
		.Padding(4.0f)
		.VAlign(VAlign_Center)
		[
			SNew(SHorizontalBox)
			.Visibility(this, &SUnityVersionControlSettings::CanCreatePlasticWorkspace)
			+SHorizontalBox::Slot()
			.FillWidth(1.0f)
			[
				SNew(SButton)
				.IsEnabled(this, &SUnityVersionControlSettings::IsReadyToCreatePlasticWorkspace)
				.Text(LOCTEXT("PlasticInitWorkspace", "Create a new Unity Version Control workspace for the current project"))
				.ToolTipText(LOCTEXT("PlasticInitWorkspace_Tooltip", "Create a new Unity Version Control repository and workspace and for the current project"))
				.OnClicked(this, &SUnityVersionControlSettings::OnClickedCreatePlasticWorkspace)
				.HAlign(HAlign_Center)
				.ContentPadding(6)
			]
		]
		// Button to add a 'ignore.conf' file on an existing Workspace
		+SVerticalBox::Slot()
		.FillHeight(2.0f)
		.Padding(2.0f)
		.VAlign(VAlign_Center)
		[
			SNew(SHorizontalBox)
			.Visibility(this, &SUnityVersionControlSettings::CanAddIgnoreFile)
			+SHorizontalBox::Slot()
			.FillWidth(1.0f)
			[
				SNew(SButton)
				.Text(LOCTEXT("CreateIgnoreFile", "Add a ignore.conf file"))
				.ToolTipText(LOCTEXT("CreateIgnoreFile_Tooltip", "Create and add a standard 'ignore.conf' file"))
				.OnClicked(this, &SUnityVersionControlSettings::OnClickedAddIgnoreFile)
				.HAlign(HAlign_Center)
			]
		]
#if ENGINE_MAJOR_VERSION == 4
	]
#endif
	];
}

EVisibility SUnityVersionControlSettings::PlasticNotAvailable() const
{
	const FUnityVersionControlProvider& Provider = FUnityVersionControlModule::Get().GetProvider();
	return Provider.IsPlasticAvailable() ? EVisibility::Collapsed : EVisibility::Visible;
}

FText SUnityVersionControlSettings::GetBinaryPathText() const
{
	return FText::FromString(FUnityVersionControlModule::Get().GetProvider().AccessSettings().GetBinaryPath());
}

void SUnityVersionControlSettings::OnBinaryPathTextCommited(const FText& InText, ETextCommit::Type InCommitType) const
{
	FUnityVersionControlProvider& Provider = FUnityVersionControlModule::Get().GetProvider();
	const bool bChanged = Provider.AccessSettings().SetBinaryPath(InText.ToString());
	if (bChanged)
	{
		// Re-Check provided cm binary path for each change
		Provider.CheckPlasticAvailability();
		if (Provider.IsPlasticAvailable())
		{
			Provider.AccessSettings().SaveSettings();
		}
	}
}

FText SUnityVersionControlSettings::GetVersions() const
{
	const FUnityVersionControlProvider& Provider = FUnityVersionControlModule::Get().GetProvider();
	return FText::FromString(Provider.GetPlasticScmVersion().String + TEXT("\t(plugin v") + Provider.GetPluginVersion() + TEXT(")"));
}

FText SUnityVersionControlSettings::GetPathToWorkspaceRoot() const
{
	return FText::FromString(FUnityVersionControlModule::Get().GetProvider().GetPathToWorkspaceRoot());
}

FText SUnityVersionControlSettings::GetUserName() const
{
	return FText::FromString(FUnityVersionControlModule::Get().GetProvider().GetUserName());
}


EVisibility SUnityVersionControlSettings::CanCreatePlasticWorkspace() const
{
	const FUnityVersionControlProvider& Provider = FUnityVersionControlModule::Get().GetProvider();
	const bool bPlasticAvailable = Provider.IsPlasticAvailable();
	const bool bPlasticWorkspaceFound = Provider.IsWorkspaceFound();
	return (bPlasticAvailable && !bPlasticWorkspaceFound) ? EVisibility::Visible : EVisibility::Collapsed;
}

bool SUnityVersionControlSettings::IsReadyToCreatePlasticWorkspace() const
{
	// Workspace Name cannot be left empty
	const bool bWorkspaceNameOk = !WorkspaceName.IsEmpty();
	// RepositoryName and ServerUrl should also be filled
	const bool bRepositoryNameOk = !RepositoryName.IsEmpty() && !ServerUrl.IsEmpty();
	// If Initial Commit is requested, checkin message cannot be empty
	const bool bInitialCommitOk = (!bAutoInitialCommit || !InitialCommitMessage.IsEmpty());
	return bWorkspaceNameOk && bRepositoryNameOk && bInitialCommitOk;
}


void SUnityVersionControlSettings::OnWorkspaceNameCommited(const FText& InText, ETextCommit::Type InCommitType)
{
	WorkspaceName = InText;
}
FText SUnityVersionControlSettings::GetWorkspaceName() const
{
	return WorkspaceName;
}

void SUnityVersionControlSettings::OnRepositoryNameCommited(const FText& InText, ETextCommit::Type InCommitType)
{
	RepositoryName = InText;
}
FText SUnityVersionControlSettings::GetRepositoryName() const
{
	return RepositoryName;
}

void SUnityVersionControlSettings::OnServerUrlCommited(const FText& InText, ETextCommit::Type InCommitType)
{
	ServerUrl = InText;
}
FText SUnityVersionControlSettings::GetServerUrl() const
{
	return ServerUrl;
}

bool SUnityVersionControlSettings::CreatePartialWorkspace() const
{
	return bCreatePartialWorkspace;
}

void SUnityVersionControlSettings::OnCheckedCreatePartialWorkspace(ECheckBoxState NewCheckedState)
{
	bCreatePartialWorkspace = (NewCheckedState == ECheckBoxState::Checked);
}

bool SUnityVersionControlSettings::CanAutoCreateIgnoreFile() const
{
	const bool bIgnoreFileFound = FPaths::FileExists(GetIgnoreFileName());
	return !bIgnoreFileFound;
}

void SUnityVersionControlSettings::OnCheckedCreateIgnoreFile(ECheckBoxState NewCheckedState)
{
	bAutoCreateIgnoreFile = (NewCheckedState == ECheckBoxState::Checked);
}

void SUnityVersionControlSettings::OnCheckedInitialCommit(ECheckBoxState NewCheckedState)
{
	bAutoInitialCommit = (NewCheckedState == ECheckBoxState::Checked);
}

void SUnityVersionControlSettings::OnInitialCommitMessageCommited(const FText& InText, ETextCommit::Type InCommitType)
{
	InitialCommitMessage = InText;
}

FText SUnityVersionControlSettings::GetInitialCommitMessage() const
{
	return InitialCommitMessage;
}


FReply SUnityVersionControlSettings::OnClickedCreatePlasticWorkspace()
{
	UE_LOG(LogSourceControl, Log, TEXT("CreatePlasticWorkspace(%s, %s, %s) CreateIgnore=%d Commit=%d"),
		*WorkspaceName.ToString(), *RepositoryName.ToString(), *ServerUrl.ToString(), bAutoCreateIgnoreFile, bAutoInitialCommit);

	// 1.a. Create a repository (if not already existing) and a workspace: launch an asynchronous MakeWorkspace operation
	LaunchMakeWorkspaceOperation();

	return FReply::Handled();
}


/// 1. Create a repository (if not already existing) and a workspace
void SUnityVersionControlSettings::LaunchMakeWorkspaceOperation()
{
	TSharedRef<FPlasticMakeWorkspace, ESPMode::ThreadSafe> MakeWorkspaceOperation = ISourceControlOperation::Create<FPlasticMakeWorkspace>();
	MakeWorkspaceOperation->WorkspaceName = WorkspaceName.ToString();
	MakeWorkspaceOperation->RepositoryName = RepositoryName.ToString();
	MakeWorkspaceOperation->ServerUrl = ServerUrl.ToString();
	MakeWorkspaceOperation->bPartialWorkspace = bCreatePartialWorkspace;

	FUnityVersionControlProvider& Provider = FUnityVersionControlModule::Get().GetProvider();
	ECommandResult::Type Result = Provider.Execute(MakeWorkspaceOperation, TArray<FString>(), EConcurrency::Asynchronous, FSourceControlOperationComplete::CreateSP(this, &SUnityVersionControlSettings::OnSourceControlOperationComplete));
	if (Result == ECommandResult::Succeeded)
	{
		DisplayInProgressNotification(MakeWorkspaceOperation->GetInProgressString());
	}
	else
	{
		DisplayFailureNotification(MakeWorkspaceOperation->GetName());
	}
}

/// 2. Add all project files to Source Control (.uproject, Config/, Content/, Source/ files and ignore.conf if any)
void SUnityVersionControlSettings::LaunchMarkForAddOperation()
{
	TSharedRef<FMarkForAdd, ESPMode::ThreadSafe> MarkForAddOperation = ISourceControlOperation::Create<FMarkForAdd>();
	FUnityVersionControlProvider& Provider = FUnityVersionControlModule::Get().GetProvider();

	// 1.b. Check the new workspace status to enable connection
	Provider.CheckPlasticAvailability();

	if (Provider.IsWorkspaceFound())
	{
		if (bAutoCreateIgnoreFile)
		{
			// 1.c Create a standard "ignore.conf" file with common patterns for a typical Blueprint & C++ project
			CreateIgnoreFile();
		}
		// 2. Add all project files to Source Control (.uproject, Config/, Content/, Source/ files and ignore.conf if any)
		const TArray<FString> ProjectFiles = GetProjectFiles();
		ECommandResult::Type Result = Provider.Execute(MarkForAddOperation, ProjectFiles, EConcurrency::Asynchronous, FSourceControlOperationComplete::CreateSP(this, &SUnityVersionControlSettings::OnSourceControlOperationComplete));
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

/// 3. Launch an asynchronous "CheckIn" operation and start another ongoing notification
void SUnityVersionControlSettings::LaunchCheckInOperation()
{
	TSharedRef<FCheckIn, ESPMode::ThreadSafe> CheckInOperation = ISourceControlOperation::Create<FCheckIn>();
	CheckInOperation->SetDescription(InitialCommitMessage);
	FUnityVersionControlProvider& Provider = FUnityVersionControlModule::Get().GetProvider();
	const TArray<FString> ProjectFiles = GetProjectFiles(); // Note: listing files and folders is only needed for the update status operation following the checkin to know on what to operate
	ECommandResult::Type Result = Provider.Execute(CheckInOperation, ProjectFiles, EConcurrency::Asynchronous, FSourceControlOperationComplete::CreateSP(this, &SUnityVersionControlSettings::OnSourceControlOperationComplete));
	if (Result == ECommandResult::Succeeded)
	{
		DisplayInProgressNotification(CheckInOperation->GetInProgressString());
	}
	else
	{
		DisplayFailureNotification(CheckInOperation->GetName());
	}
}

/// Delegate called when a source control operation has completed: launch the next one and manage notifications
void SUnityVersionControlSettings::OnSourceControlOperationComplete(const FSourceControlOperationRef& InOperation, ECommandResult::Type InResult)
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

	// Launch the following asynchronous operation
	if ((InOperation->GetName() == "MakeWorkspace") && (InResult == ECommandResult::Succeeded) && bAutoInitialCommit)
	{
		// 2. Add .uproject, Config/, Content/ and Source/ files (and ignore.conf if any)
		LaunchMarkForAddOperation();
	}
	else if ((InOperation->GetName() == "MarkForAdd") && (InResult == ECommandResult::Succeeded) && bAutoInitialCommit)
	{
		// 3. optional initial Asynchronous commit with custom message: launch a "CheckIn" Operation
		LaunchCheckInOperation();
	}
}

// Display an ongoing notification during the whole operation
void SUnityVersionControlSettings::DisplayInProgressNotification(const FText& InOperationInProgressString)
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
void SUnityVersionControlSettings::RemoveInProgressNotification()
{
	if (OperationInProgressNotification.IsValid())
	{
		OperationInProgressNotification.Pin()->ExpireAndFadeout();
		OperationInProgressNotification.Reset();
	}
}

// Display a temporary success notification at the end of the operation
void SUnityVersionControlSettings::DisplaySuccessNotification(const FName& InOperationName)
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
void SUnityVersionControlSettings::DisplayFailureNotification(const FName& InOperationName)
{
	const FText NotificationText = FText::Format(LOCTEXT("InitWorkspace_Failure", "Error: {0} operation failed!"), FText::FromName(InOperationName));
	FNotificationInfo Info(NotificationText);
	Info.ExpireDuration = 8.0f;
	FSlateNotificationManager::Get().AddNotification(Info);
	UE_LOG(LogSourceControl, Error, TEXT("%s"), *NotificationText.ToString());
}

/** Delegate to check for presence of an ignore.conf file to an existing Unity Version Control workspace */
EVisibility SUnityVersionControlSettings::CanAddIgnoreFile() const
{
	const bool bPlasticWorkspaceFound = FUnityVersionControlModule::Get().GetProvider().IsWorkspaceFound();
	const bool bIgnoreFileFound = FPaths::FileExists(GetIgnoreFileName());
	return (bPlasticWorkspaceFound && !bIgnoreFileFound) ? EVisibility::Visible : EVisibility::Collapsed;
}

/** Delegate to add an ignore.conf file to an existing Unity Version Control workspace */
FReply SUnityVersionControlSettings::OnClickedAddIgnoreFile() const
{
	if (CreateIgnoreFile())
	{
		// Add ignore.conf to Unity Version Control
		TArray<FString> InfoMessages;
		TArray<FString> ErrorMessages;
		TArray<FString> Parameters;
		Parameters.Add(TEXT("-R"));
		TArray<FString> Files;
		Files.Add(TEXT("ignore.conf"));
		UnityVersionControlUtils::RunCommand(TEXT("add"), Parameters, Files, InfoMessages, ErrorMessages);
	}
	return FReply::Handled();
}

void SUnityVersionControlSettings::OnCheckedUpdateStatusAtStartup(ECheckBoxState NewCheckedState)
{
	FUnityVersionControlSettings& PlasticSettings = FUnityVersionControlModule::Get().GetProvider().AccessSettings();
	PlasticSettings.SetUpdateStatusAtStartup(NewCheckedState == ECheckBoxState::Checked);
	PlasticSettings.SaveSettings();
}

ECheckBoxState SUnityVersionControlSettings::IsUpdateStatusAtStartupChecked() const
{
	FUnityVersionControlSettings& PlasticSettings = FUnityVersionControlModule::Get().GetProvider().AccessSettings();
	return PlasticSettings.GetUpdateStatusAtStartup() ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

void SUnityVersionControlSettings::OnCheckedUpdateStatusOtherBranches(ECheckBoxState NewCheckedState)
{
	FUnityVersionControlSettings& PlasticSettings = FUnityVersionControlModule::Get().GetProvider().AccessSettings();
	PlasticSettings.SetUpdateStatusOtherBranches(NewCheckedState == ECheckBoxState::Checked);
	PlasticSettings.SaveSettings();
}

ECheckBoxState SUnityVersionControlSettings::IsUpdateStatusOtherBranchesChecked() const
{
	FUnityVersionControlSettings& PlasticSettings = FUnityVersionControlModule::Get().GetProvider().AccessSettings();
	return PlasticSettings.GetUpdateStatusOtherBranches() ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

void SUnityVersionControlSettings::OnCheckedEnableVerboseLogs(ECheckBoxState NewCheckedState)
{
	FUnityVersionControlSettings& PlasticSettings = FUnityVersionControlModule::Get().GetProvider().AccessSettings();
	PlasticSettings.SetEnableVerboseLogs(NewCheckedState == ECheckBoxState::Checked);
	PlasticSettings.SaveSettings();

	UnityVersionControlUtils::SwitchVerboseLogs(NewCheckedState == ECheckBoxState::Checked);
}

ECheckBoxState SUnityVersionControlSettings::IsEnableVerboseLogsChecked() const
{
	FUnityVersionControlSettings& PlasticSettings = FUnityVersionControlModule::Get().GetProvider().AccessSettings();
	return PlasticSettings.GetEnableVerboseLogs() ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

/** Path to the "ignore.conf" file */
const FString SUnityVersionControlSettings::GetIgnoreFileName() const
{
	const FString PathToWorkspaceRoot = FUnityVersionControlModule::Get().GetProvider().GetPathToWorkspaceRoot();
	const FString IgnoreFileName = FPaths::Combine(*PathToWorkspaceRoot, TEXT("ignore.conf"));
	return IgnoreFileName;
}

/** Create a standard "ignore.conf" file with common patterns for a typical Blueprint & C++ project */
bool SUnityVersionControlSettings::CreateIgnoreFile() const
{
	const FString IgnoreFileContent = TEXT("Binaries\nBuild\nDerivedDataCache\nIntermediate\nSaved\nScript\nenc_temp_folder\n.idea\n.vscode\n.vs\n.ignore\n*.VC.db\n*.opensdf\n*.opendb\n*.sdf\n*.sln\n*.suo\n*.code-workspace\n*.xcodeproj\n*.xcworkspace");
	return FFileHelper::SaveStringToFile(IgnoreFileContent, *GetIgnoreFileName(), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
}

/** List of files to add to Source Control (.uproject, Config/, Content/, Source/ files and ignore.conf if any) */
TArray<FString> SUnityVersionControlSettings::GetProjectFiles() const
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
