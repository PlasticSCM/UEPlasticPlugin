// Copyright (c) 2024 Unity Technologies

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
#include "Styling/SlateTypes.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SHyperlink.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SExpandableArea.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/SBoxPanel.h"
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
	WorkspaceParams.bAutoInitialCommit = true;

	WorkspaceParams.InitialCommitMessage = LOCTEXT("InitialCommitMessage", "Initial checkin");

	const FUnityVersionControlProvider& Provider = FUnityVersionControlModule::Get().GetProvider();

	const TMap<FString, FString>& Profiles = Provider.GetProfiles();
	for (const auto& Profile : Profiles)
	{
		ServerNames.Add(FText::FromString(Profile.Key));
	}

	// If no workspace found, offer to create a new one on the selected server
	if (Provider.IsPlasticAvailable() && !Provider.IsWorkspaceFound())
	{
		// Use the configured list of profiles from the provider so we can list both servers & associated user name
		// Note: this doesn't need any of these to be editable, if they are missing the user needs to use the Desktop application to configure them.
		if (!Provider.GetServerUrl().IsEmpty())
		{
			OnServerSelected(FText::FromString(Provider.GetServerUrl()));
		}
		else
		{
			OnServerSelected(FText::FromString(UnityVersionControlUtils::GetConfigDefaultRepServer()));
		}
	}

	if (FApp::HasProjectName())
	{
		WorkspaceParams.WorkspaceName = FText::FromString(FApp::GetProjectName());
		WorkspaceParams.RepositoryName = WorkspaceParams.WorkspaceName;
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
		// Path to the CLI
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
            .Padding(0.0f, 3.0f)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("PlasticVersions", "Unity Version Control"))
				.Font(Font)
			]
			+SHorizontalBox::Slot()
			.FillWidth(0.5f)
			[
				SNew(SEditableTextBox)
				.Text(this, &SUnityVersionControlSettings::GetBinaryPathText)
				.HintText(LOCTEXT("BinaryPathLabel", "Path to the Unity Version Control 'cm' executable"))
				.OnTextCommitted(this, &SUnityVersionControlSettings::OnBinaryPathTextCommited)
				.Font(Font)
			]
			+SHorizontalBox::Slot()
			.FillWidth(1.5f)
            .Padding(4.0f, 3.0f)
			[
				SNew(STextBlock)
				.Text(this, &SUnityVersionControlSettings::GetVersions)
				.Font(Font)
			]
		]
		// Unity Version Control command line tool not available warning and download link
		+SVerticalBox::Slot()
		.AutoHeight()
		.Padding(2.0f)
		[
			SNew(STextBlock)
			.Visibility(this, &SUnityVersionControlSettings::PlasticNotAvailable)
			.ToolTipText(LOCTEXT("PlasticNotAvailable_Tooltip", "Failed to launch Unity Version Control 'cm' command line tool.\nYou need to install it and make sure it is correctly configured with your credentials."))
			.Text(LOCTEXT("PlasticNotAvailable", "Unity Version Control Command Line tool 'cm' failed to start."))
			.Font(Font)
			.ColorAndOpacity(FLinearColor::Red)
		]
		+SVerticalBox::Slot()
		.AutoHeight()
		.Padding(2.0f)
		[
			SNew(SHorizontalBox)
			+SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SHyperlink)
				.Visibility(this, &SUnityVersionControlSettings::PlasticNotAvailable)
				.ToolTipText(LOCTEXT("PlasticDownload_Tooltip", "Download Unity Version Control (Plastic SCM)"))
				.Text(LOCTEXT("PlasticDownload", "https://www.plasticscm.com/download/downloadinstaller/..."))
				.OnNavigate(FSimpleDelegate::CreateLambda([]()
				{
#if PLATFORM_WINDOWS
					FPlatformProcess::LaunchURL(TEXT("https://www.plasticscm.com/download/downloadinstaller/last/plasticscm/windows/cloudedition"), NULL, NULL);
#elif PLATFORM_MAC
					FPlatformProcess::LaunchURL(TEXT("https://www.plasticscm.com/download/downloadinstaller/last/plasticscm/macosx/cloudedition"), NULL, NULL);
#elif PLATFORM_LINUX
					FPlatformProcess::LaunchURL(TEXT("https://www.plasticscm.com/plastic-for-linux"), NULL, NULL);
#endif
				}))
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
		// No Workspace found - Separator and explanation text
		+SVerticalBox::Slot()
		.AutoHeight()
		.Padding(2.0f, 4.0f)
		.VAlign(VAlign_Center)
		[
			SNew(SSeparator)
			.Visibility(this, &SUnityVersionControlSettings::CanCreatePlasticWorkspace)
		]
		+SVerticalBox::Slot()
		.AutoHeight()
		.Padding(2.0f, 4.0f)
		[
			SNew(STextBlock)
			.Visibility(this, &SUnityVersionControlSettings::CanCreatePlasticWorkspace)
			.ToolTipText(LOCTEXT("WorkspaceNotFound_Tooltip", "No Workspace found at the level or above the current Unreal project. Use the form to create a new one."))
			.Text(LOCTEXT("WorkspaceNotFound", "Create a Workspace for your Unreal project:"))
			.Font(Font)
		]
		// Repository specification if Workspace found
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(2.0f, 4.0f)
		.VAlign(VAlign_Center)
		[
			SNew(SHorizontalBox)
			.Visibility(this, &SUnityVersionControlSettings::IsWorkspaceFound)
			.ToolTipText(this, &SUnityVersionControlSettings::GetRepositorySpec)
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			[
				SNew(STextBlock)
					.Text(LOCTEXT("RepositorySpecification", "Repository"))
					.Font(Font)
			]
			+ SHorizontalBox::Slot()
			.FillWidth(2.0f)
			[
				SNew(STextBlock)
					.Text(this, &SUnityVersionControlSettings::GetRepositorySpec)
					.Font(Font)
			]
		]

		// User Name configured for the selected server
		+SVerticalBox::Slot()
		.AutoHeight()
		.Padding(2.0f, 4.0f)
		.VAlign(VAlign_Center)
		[
			SNew(SHorizontalBox)
			.ToolTipText(LOCTEXT("PlasticUserName_Tooltip", "User name configured for the selected Unity Version Control server"))
			+SHorizontalBox::Slot()
			.FillWidth(1.0f)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("PlasticUserName", "User name"))
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

		// Organization Name or Server URL address:port Dropdown
		+SVerticalBox::Slot()
		.AutoHeight()
		.Padding(2.0f)
		.VAlign(VAlign_Center)
		[
			SNew(SHorizontalBox)
			.Visibility(this, &SUnityVersionControlSettings::CanSelectServer)
			.ToolTipText(LOCTEXT("ServerUrl_Tooltip", "Enter the cloud organization (eg. YourOrganization@cloud, YourOrganization@unity, local) or the Server URL in the form address:port or ssl://ip:port (eg localhost:8087)"))
			+SHorizontalBox::Slot()
			.FillWidth(1.0f)
            .Padding(0.0f, 4.0f)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("ServerUrl", "Organization or server"))
				.Font(Font)
			]
			+SHorizontalBox::Slot()
			.FillWidth(2.0f)
			.Padding(2.0f, 0.0f, 0.0f, 0.0f)
			[
				SNew(SComboButton)
				.OnGetMenuContent(this, &SUnityVersionControlSettings::BuildServerDropDownMenu)
				.IsEnabled_Lambda([this] { return !bGetProjectsInProgress; })
				.ButtonContent()
				[
					SNew(STextBlock)
					.Text(this, &SUnityVersionControlSettings::GetServerUrl)
					.Font(Font)
				]
			]
		]
		// No Known Server configured - Error message and explanation
		+SVerticalBox::Slot()
		.AutoHeight()
		.Padding(2.0f)
		[
			SNew(STextBlock)
			.Visibility(this, &SUnityVersionControlSettings::NoServerToSelect)
			.ToolTipText(LOCTEXT("NoKnownServer_Tooltip", "You don't have any server configured.\nYou need to launch the Desktop application and make sure it is correctly configured with your credentials."))
			.Text(LOCTEXT("NoKnownServer", "You don't have any server configured."))
			.Font(Font)
			.ColorAndOpacity(FLinearColor::Red)
		]

		// Organization Project Dropdown
		+SVerticalBox::Slot()
		.AutoHeight()
		.Padding(2.0f)
		.VAlign(VAlign_Center)
		[
			SNew(SHorizontalBox)
			.Visibility(this, &SUnityVersionControlSettings::CanSelectProject)
            .ToolTipText(LOCTEXT("ProjectName_Tooltip", "Select the name of the Project to use"))
			+SHorizontalBox::Slot()
			.FillWidth(1.0f)
            .Padding(0.0f, 4.0f)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("ProjectName", "Organization's project"))
				.Font(Font)
			]
			+SHorizontalBox::Slot()
			.FillWidth(2.0f)
			.Padding(2.0f, 0.0f, 0.0f, 0.0f)
			[
				SNew(SComboButton)
				.OnGetMenuContent(this, &SUnityVersionControlSettings::BuildProjectDropDownMenu)
				.ButtonContent()
				[
					SNew(STextBlock)
					.Text(this, &SUnityVersionControlSettings::GetProjectName)
					.Font(Font)
				]
			]
		]
		// No Project Explanation text
		+SVerticalBox::Slot()
		.AutoHeight()
		.Padding(2.0f)
		[
			SNew(STextBlock)
			.Visibility_Lambda([this] { return bGetProjectsInProgress ? EVisibility::Visible : EVisibility::Collapsed; })
			.Text(LOCTEXT("GetProjectsInProgress", "Getting the list of projects in this Unity Organization..."))
			.Font(Font)
		]
		// No Project Explanation text
		+SVerticalBox::Slot()
		.AutoHeight()
		.Padding(2.0f)
		[
			SNew(STextBlock)
			.Visibility(this, &SUnityVersionControlSettings::NoProjectToSelect)
			.ToolTipText(LOCTEXT("NoProject_Tooltip", "You don't have access to any Project in this Unity organization.\nYou need to use the Unity Dashboard to make sure you have access to a project in the selected Unity organization."))
			.Text(LOCTEXT("NoProject", "You don't have access to any Project in this Unity organization."))
			.Font(Font)
			.ColorAndOpacity(FLinearColor::Red)
		]

		// Repository Name
		+SVerticalBox::Slot()
		.AutoHeight()
		.Padding(2.0f)
		.VAlign(VAlign_Center)
		[
			SNew(SHorizontalBox)
			.Visibility(this, &SUnityVersionControlSettings::CanCreatePlasticWorkspace)
            .ToolTipText(LOCTEXT("RepositoryName_Tooltip", "Enter the Name of the Repository to use or create"))
			+SHorizontalBox::Slot()
			.FillWidth(1.0f)
            .Padding(0.0f, 3.0f)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("RepositoryName", "Repository name"))
				.Font(Font)
			]
			+SHorizontalBox::Slot()
			.FillWidth(2.0f)
			.Padding(2.0f, 0.0f, 0.0f, 0.0f)
			[
				SNew(SEditableTextBox)
				.Text(this, &SUnityVersionControlSettings::GetRepositoryName)
				.HintText(LOCTEXT("RepositoryName_Hint", "Name of the Repository to use or create"))
				.OnTextCommitted(this, &SUnityVersionControlSettings::OnRepositoryNameCommited)
				.Font(Font)
			]
		]
		// Workspace Name
		+SVerticalBox::Slot()
		.AutoHeight()
		.Padding(2.0f)
		.VAlign(VAlign_Center)
		[
			SNew(SHorizontalBox)
			.Visibility(this, &SUnityVersionControlSettings::CanCreatePlasticWorkspace)
            .ToolTipText(LOCTEXT("WorkspaceName_Tooltip", "Enter the Name of the new Workspace to create"))
			+SHorizontalBox::Slot()
			.FillWidth(1.0f)
            .Padding(0.0f, 3.0f)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("WorkspaceName", "Workspace name"))
				.Font(Font)
			]
			+SHorizontalBox::Slot()
			.FillWidth(2.0f)
			.Padding(2.0f, 0.0f, 0.0f, 0.0f)
			[
				SNew(SEditableTextBox)
				.Text(this, &SUnityVersionControlSettings::GetWorkspaceName)
				.HintText(LOCTEXT("WorkspaceName_Hint", "Name of the Workspace to create"))
				.OnTextCommitted(this, &SUnityVersionControlSettings::OnWorkspaceNameCommited)
				.Font(Font)
			]
		]
		// Option to create a Partial/Gluon Workspace designed to only sync selected files and allow to check-in when the workspace is not up to date.
		+SVerticalBox::Slot()
		.AutoHeight()
		.Padding(2.0f)
		.VAlign(VAlign_Center)
		[
			SNew(SCheckBox)
			.Visibility(this, &SUnityVersionControlSettings::CanCreatePlasticWorkspace)
			.ToolTipText(LOCTEXT("CreatePartialWorkspace_Tooltip", "Create the new workspace in Gluon/partial mode, designed for artists, instead of a Full/regular workspace for developpers."))
			.IsChecked(WorkspaceParams.bCreatePartialWorkspace)
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
		// Option to Make the initial checkin of the whole project
		+SVerticalBox::Slot()
		.AutoHeight()
		.Padding(2.0f)
		.VAlign(VAlign_Center)
		[
			SNew(SHorizontalBox)
			.Visibility(this, &SUnityVersionControlSettings::CanCreatePlasticWorkspace)
			.ToolTipText(LOCTEXT("InitialCommit_Tooltip", "Make the initial checkin of the whole project"))
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
			[
				SNew(SMultiLineEditableTextBox)
				.Text(this, &SUnityVersionControlSettings::GetInitialCommitMessage)
				.HintText(LOCTEXT("InitialCommitMessage_Hint", "Message for the initial checkin"))
				.OnTextCommitted(this, &SUnityVersionControlSettings::OnInitialCommitMessageCommited)
				.Font(Font)
			]
		]

		// Advanced runtime Settings expandable area
		+SVerticalBox::Slot()
		.VAlign(VAlign_Top)
		.AutoHeight()
		.Padding(0.0f)
		[
			SNew(SExpandableArea)
			.BorderImage(FAppStyle::Get().GetBrush("NoBorder"))
			.InitiallyCollapsed(true)
			.HeaderContent()
			[
				SNew(STextBlock)
				.Text(LOCTEXT("AdvancedRuntimeSettings", "Advanced runtime Settings"))
			]
			.BodyContent()
			[
				SNew(SBorder)
				.BorderImage(FAppStyle::Get().GetBrush("NoBorder"))
				.Padding(0)
				[
					SNew(SVerticalBox)
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
					// Option for the View Changes (Changelists) window to also show locally Changed and Private files
					+SVerticalBox::Slot()
					.AutoHeight()
					.Padding(2.0f)
					.VAlign(VAlign_Center)
					[
						SNew(SCheckBox)
						.ToolTipText(LOCTEXT("ViewLocalChanges_Tooltip", "Enable the \"View Changes\" window to search for and show locally Changed and Private files (can be slow)."))
						.IsChecked(SUnityVersionControlSettings::IsViewLocalChangesChecked())
						.OnCheckStateChanged(this, &SUnityVersionControlSettings::OnCheckedViewLocalChanges)
						[
							SNew(STextBlock)
							.Text(LOCTEXT("ViewLocalChanges", "Show local Changes in the \"View Changes\" window."))
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
				]
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
                .ContentPadding(6.0f)
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


EVisibility SUnityVersionControlSettings::IsWorkspaceFound() const
{
	const FUnityVersionControlProvider& Provider = FUnityVersionControlModule::Get().GetProvider();
	const bool bPlasticAvailable = Provider.IsPlasticAvailable();
	const bool bPlasticWorkspaceFound = Provider.IsWorkspaceFound();
	return (bPlasticAvailable && bPlasticWorkspaceFound) ? EVisibility::Visible : EVisibility::Collapsed;
}

EVisibility SUnityVersionControlSettings::CanCreatePlasticWorkspace() const
{
	const FUnityVersionControlProvider& Provider = FUnityVersionControlModule::Get().GetProvider();
	const bool bPlasticAvailable = Provider.IsPlasticAvailable();
	const bool bPlasticWorkspaceFound = Provider.IsWorkspaceFound();
	return (bPlasticAvailable && !bPlasticWorkspaceFound) ? EVisibility::Visible : EVisibility::Collapsed;
}

EVisibility SUnityVersionControlSettings::CanSelectServer() const
{
	const FUnityVersionControlProvider& Provider = FUnityVersionControlModule::Get().GetProvider();
	const bool bPlasticAvailable = Provider.IsPlasticAvailable();
	const bool bPlasticWorkspaceFound = Provider.IsWorkspaceFound();
	const bool bHasKnownServers = !ServerNames.IsEmpty();
	return (bPlasticAvailable && !bPlasticWorkspaceFound && bHasKnownServers) ? EVisibility::Visible : EVisibility::Collapsed;
}

EVisibility SUnityVersionControlSettings::NoServerToSelect() const
{
	const FUnityVersionControlProvider& Provider = FUnityVersionControlModule::Get().GetProvider();
	const bool bPlasticAvailable = Provider.IsPlasticAvailable();
	const bool bPlasticWorkspaceFound = Provider.IsWorkspaceFound();
	const bool bHasKnownServers = !ServerNames.IsEmpty();
	return (bPlasticAvailable && !bPlasticWorkspaceFound && !bHasKnownServers) ? EVisibility::Visible : EVisibility::Collapsed;
}


EVisibility SUnityVersionControlSettings::CanSelectProject() const
{
	const FUnityVersionControlProvider& Provider = FUnityVersionControlModule::Get().GetProvider();
	const bool bPlasticAvailable = Provider.IsPlasticAvailable();
	const bool bPlasticWorkspaceFound = Provider.IsWorkspaceFound();
	const bool bIsUnityOrganization = UnityVersionControlUtils::IsUnityOrganization(WorkspaceParams.ServerUrl.ToString());
	const bool bHasProjects = !ProjectNames.IsEmpty();
	return (bPlasticAvailable && !bPlasticWorkspaceFound && bIsUnityOrganization && bHasProjects) ? EVisibility::Visible : EVisibility::Collapsed;
}

EVisibility SUnityVersionControlSettings::NoProjectToSelect() const
{
	const FUnityVersionControlProvider& Provider = FUnityVersionControlModule::Get().GetProvider();
	const bool bPlasticAvailable = Provider.IsPlasticAvailable();
	const bool bPlasticWorkspaceFound = Provider.IsWorkspaceFound();
	const bool bIsUnityOrganization = UnityVersionControlUtils::IsUnityOrganization(WorkspaceParams.ServerUrl.ToString());
	const bool bHasProjects = !ProjectNames.IsEmpty();
	return (bPlasticAvailable && !bPlasticWorkspaceFound && bIsUnityOrganization && !bHasProjects && !bGetProjectsInProgress) ? EVisibility::Visible : EVisibility::Collapsed;
}

bool SUnityVersionControlSettings::IsReadyToCreatePlasticWorkspace() const
{
	// Workspace Name cannot be left empty
	const bool bWorkspaceNameOk = !WorkspaceParams.WorkspaceName.IsEmpty();
	// RepositoryName and ServerUrl should also be filled
	const bool bRepositoryNameOk = !WorkspaceParams.RepositoryName.IsEmpty() && !WorkspaceParams.ServerUrl.IsEmpty();
	// And the Project is required if the server is a Unity Organization
	const bool bProjectNameOk = !UnityVersionControlUtils::IsUnityOrganization(WorkspaceParams.ServerUrl.ToString()) || !WorkspaceParams.ProjectName.IsEmpty();
	// If Initial Commit is requested, checkin message cannot be empty
	const bool bInitialCommitOk = (!WorkspaceParams.bAutoInitialCommit || !WorkspaceParams.InitialCommitMessage.IsEmpty());
	return bWorkspaceNameOk && bRepositoryNameOk && bProjectNameOk && bInitialCommitOk;
}

FText SUnityVersionControlSettings::GetRepositorySpec() const
{
	const FUnityVersionControlProvider& Provider = FUnityVersionControlModule::Get().GetProvider();
	return FText::FromString(FString::Printf(TEXT("%s@%s"), *Provider.GetRepositoryName(), *Provider.GetServerUrl()));
}

FText SUnityVersionControlSettings::GetServerUrl() const
{
	return WorkspaceParams.ServerUrl;
}
TSharedRef<SWidget> SUnityVersionControlSettings::BuildServerDropDownMenu()
{
	FMenuBuilder MenuBuilder(true, NULL);

	for (const FText& ServerName : ServerNames)
	{
		FUIAction MenuAction(FExecuteAction::CreateSP(this, &SUnityVersionControlSettings::OnServerSelected, ServerName));
		MenuBuilder.AddMenuEntry(ServerName, ServerName, FSlateIcon(), MenuAction);
	}

	return MenuBuilder.MakeWidget();
}
void SUnityVersionControlSettings::OnServerSelected(const FText InServerName)
{
	if (WorkspaceParams.ServerUrl.EqualTo(InServerName))
		return;

	WorkspaceParams.ServerUrl = InServerName;
	WorkspaceParams.ProjectName = FText::GetEmpty();

	UE_LOG(LogSourceControl, Verbose, TEXT("OnServerSelected(%s)"), *WorkspaceParams.ServerUrl.ToString());

	FUnityVersionControlModule::Get().GetProvider().UpdateServerUrl(WorkspaceParams.ServerUrl.ToString());

	// Get the Projects for the Unity Organization
	if (UnityVersionControlUtils::IsUnityOrganization(WorkspaceParams.ServerUrl.ToString()))
	{
		ProjectNames.Empty();

		// Launch an asynchronous GetProjects operation
		TSharedRef<FPlasticGetProjects, ESPMode::ThreadSafe> GetProjectsOperation = ISourceControlOperation::Create<FPlasticGetProjects>();
		GetProjectsOperation->ServerUrl = WorkspaceParams.ServerUrl.ToString();
		FUnityVersionControlProvider& Provider = FUnityVersionControlModule::Get().GetProvider();
		ECommandResult::Type Result = Provider.Execute(GetProjectsOperation, TArray<FString>(), EConcurrency::Asynchronous, FSourceControlOperationComplete::CreateRaw(this, &SUnityVersionControlSettings::OnGetProjectsOperationComplete));
		if (Result == ECommandResult::Succeeded)
		{
			bGetProjectsInProgress = true;
		}
	}
}
void SUnityVersionControlSettings::OnGetProjectsOperationComplete(const FSourceControlOperationRef& InOperation, ECommandResult::Type InResult)
{
	bGetProjectsInProgress = false;

	if (InResult == ECommandResult::Succeeded)
	{
		TSharedRef<FPlasticGetProjects, ESPMode::ThreadSafe> GetProjectsOperation = StaticCastSharedRef<FPlasticGetProjects>(InOperation);

		UE_LOG(LogSourceControl, Verbose, TEXT("OnGetProjectsOperationComplete: %d projects in %s"), GetProjectsOperation->ProjectNames.Num(), *GetProjectsOperation->ServerUrl);

		// Sort project alphanumerically
		GetProjectsOperation->ProjectNames.Sort();
		ProjectNames.Reserve(GetProjectsOperation->ProjectNames.Num());
		for (FString& Project : GetProjectsOperation->ProjectNames)
		{
			ProjectNames.Add(FText::FromString(Project));
		}
		// Try to find an existing Unity project with the same name as the Unreal project (that is, the repository name)
		int32 Index;
		if (GetProjectsOperation->ProjectNames.Find(WorkspaceParams.RepositoryName.ToString(), Index))
		{
			WorkspaceParams.ProjectName = ProjectNames[Index];
		}
		else
		{
			WorkspaceParams.ProjectName = ProjectNames[0];
		}
	}
}

FText SUnityVersionControlSettings::GetProjectName() const
{
	return WorkspaceParams.ProjectName;
}
void SUnityVersionControlSettings::OnProjectSelected(const FText InProjectName)
{
	WorkspaceParams.ProjectName = InProjectName;
}
TSharedRef<SWidget> SUnityVersionControlSettings::BuildProjectDropDownMenu()
{
	FMenuBuilder MenuBuilder(true, NULL);

	for (const FText& ProjectName : ProjectNames)
	{
		FUIAction MenuAction(FExecuteAction::CreateSP(this, &SUnityVersionControlSettings::OnProjectSelected, ProjectName));
		MenuBuilder.AddMenuEntry(ProjectName, ProjectName, FSlateIcon(), MenuAction);
	}

	return MenuBuilder.MakeWidget();
}

void SUnityVersionControlSettings::OnRepositoryNameCommited(const FText& InText, ETextCommit::Type InCommitType)
{
	WorkspaceParams.RepositoryName = InText;
}
FText SUnityVersionControlSettings::GetRepositoryName() const
{
	return WorkspaceParams.RepositoryName;
}

void SUnityVersionControlSettings::OnWorkspaceNameCommited(const FText& InText, ETextCommit::Type InCommitType)
{
	WorkspaceParams.WorkspaceName = InText;
}
FText SUnityVersionControlSettings::GetWorkspaceName() const
{
	return WorkspaceParams.WorkspaceName;
}

bool SUnityVersionControlSettings::CreatePartialWorkspace() const
{
	return WorkspaceParams.bCreatePartialWorkspace;
}

void SUnityVersionControlSettings::OnCheckedCreatePartialWorkspace(ECheckBoxState NewCheckedState)
{
	WorkspaceParams.bCreatePartialWorkspace = (NewCheckedState == ECheckBoxState::Checked);
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
	WorkspaceParams.bAutoInitialCommit = (NewCheckedState == ECheckBoxState::Checked);
}

void SUnityVersionControlSettings::OnInitialCommitMessageCommited(const FText& InText, ETextCommit::Type InCommitType)
{
	WorkspaceParams.InitialCommitMessage = InText;
}

FText SUnityVersionControlSettings::GetInitialCommitMessage() const
{
	return WorkspaceParams.InitialCommitMessage;
}


FReply SUnityVersionControlSettings::OnClickedCreatePlasticWorkspace()
{
	UE_LOG(LogSourceControl, Log, TEXT("CreatePlasticWorkspace(%s, %s, %s, %s) PartialWorkspace=%d CreateIgnore=%d Commit=%d"),
		*WorkspaceParams.ServerUrl.ToString(), *WorkspaceParams.ProjectName.ToString(), *WorkspaceParams.RepositoryName.ToString(), *WorkspaceParams.WorkspaceName.ToString(),
		WorkspaceParams.bCreatePartialWorkspace, bAutoCreateIgnoreFile, WorkspaceParams.bAutoInitialCommit);

	if (bAutoCreateIgnoreFile)
	{
		// 1. Create a standard "ignore.conf" file with common patterns for a typical Blueprint & C++ project
		CreateIgnoreFile();
	}

	// 2. Create a repository (if not already existing) and a workspace: launch an asynchronous MakeWorkspace operation
	FUnityVersionControlModule::Get().GetWorkspaceCreation().MakeWorkspace(WorkspaceParams);

	return FReply::Handled();
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

void SUnityVersionControlSettings::OnCheckedViewLocalChanges(ECheckBoxState NewCheckedState)
{
	FUnityVersionControlSettings& PlasticSettings = FUnityVersionControlModule::Get().GetProvider().AccessSettings();
	PlasticSettings.SetViewLocalChanges(NewCheckedState == ECheckBoxState::Checked);
	PlasticSettings.SaveSettings();
}

ECheckBoxState SUnityVersionControlSettings::IsViewLocalChangesChecked() const
{
	FUnityVersionControlSettings& PlasticSettings = FUnityVersionControlModule::Get().GetProvider().AccessSettings();
	return PlasticSettings.GetViewLocalChanges() ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
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
	const FString IgnoreFileContent = TEXT("Binaries\nDerivedDataCache\nIntermediate\nSaved\nScript\nenc_temp_folder\n.idea\n.vscode\n.vs\n.ignore\n*.VC.db\n*.opensdf\n*.opendb\n*.sdf\n*.sln\n*.suo\n*.code-workspace\n*.xcodeproj\n*.xcworkspace\n*.private.*");
	return FFileHelper::SaveStringToFile(IgnoreFileContent, *GetIgnoreFileName(), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
}

#undef LOCTEXT_NAMESPACE
