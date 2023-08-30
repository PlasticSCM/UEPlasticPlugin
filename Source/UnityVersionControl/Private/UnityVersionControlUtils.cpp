// Copyright (c) 2023 Unity Technologies

#include "UnityVersionControlUtils.h"

#include "UnityVersionControlCommand.h"
#include "UnityVersionControlModule.h"
#include "UnityVersionControlProjectSettings.h"
#include "UnityVersionControlProvider.h"
#include "UnityVersionControlSettings.h"
#include "UnityVersionControlShell.h"
#include "UnityVersionControlState.h"
#include "UnityVersionControlVersions.h"
#include "ISourceControlModule.h"

#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "XmlParser.h"
#include "SoftwareVersion.h"
#include "ScopedTempFile.h"

#include "Runtime/Launch/Resources/Version.h"
#if ENGINE_MAJOR_VERSION == 5
#include "UnityVersionControlChangelist.h"
#include "UnityVersionControlChangelistState.h"
#endif

namespace UnityVersionControlUtils
{

#define FILE_STATUS_SEPARATOR TEXT(";")

// Run a command and return the result as raw strings
bool RunCommand(const FString& InCommand, const TArray<FString>& InParameters, const TArray<FString>& InFiles, FString& OutResults, FString& OutErrors)
{
	return UnityVersionControlShell::RunCommand(InCommand, InParameters, InFiles, OutResults, OutErrors);
}

// Run a command with basic parsing or results & errors from the cm command line process
bool RunCommand(const FString& InCommand, const TArray<FString>& InParameters, const TArray<FString>& InFiles, TArray<FString>& OutResults, TArray<FString>& OutErrorMessages)
{
	FString Results;
	FString Errors;

	const bool bResult = UnityVersionControlShell::RunCommand(InCommand, InParameters, InFiles, Results, Errors);

	Results.ParseIntoArray(OutResults, UnityVersionControlShell::pchDelim, true);
	Errors.ParseIntoArray(OutErrorMessages, UnityVersionControlShell::pchDelim, true);

	return bResult;
}

const FSoftwareVersion& GetOldestSupportedPlasticScmVersion()
{
	return UnityVersionControlVersions::OldestSupported;
}

FString FindPlasticBinaryPath()
{
#if PLATFORM_WINDOWS
	return FString(TEXT("cm"));
#else
	return FString(TEXT("/usr/bin/cm"));
#endif
}

// Find the root of the workspace, looking from the provided path and upward in its parent directories.
bool GetWorkspacePath(const FString& InPath, FString& OutWorkspaceRoot)
{
	TArray<FString> Results;
	TArray<FString> ErrorMessages;
	TArray<FString> Parameters;
	Parameters.Add(TEXT("--format={wkpath}"));
	Parameters.Add(TEXT("."));
	const bool bFound = RunCommand(TEXT("getworkspacefrompath"), Parameters, TArray<FString>(), Results, ErrorMessages);
	if (bFound && Results.Num() > 0)
	{
		OutWorkspaceRoot = MoveTemp(Results[0]);
		FPaths::NormalizeDirectoryName(OutWorkspaceRoot);
		OutWorkspaceRoot.AppendChar(TEXT('/'));
	}
	else
	{
		OutWorkspaceRoot = InPath; // If not found, return the provided dir as best possible root.
	}
	return bFound;
}

// This is called once by FUnityVersionControlProvider::CheckPlasticAvailability()
bool GetPlasticScmVersion(FSoftwareVersion& OutPlasticScmVersion)
{
	TArray<FString> Results;
	TArray<FString> ErrorMessages;
	const bool bResult = RunCommand(TEXT("version"), TArray<FString>(), TArray<FString>(), Results, ErrorMessages);
	if (bResult && Results.Num() > 0)
	{
		OutPlasticScmVersion = FSoftwareVersion(MoveTemp(Results[0]));
		return true;
	}
	return false;
}

bool GetCmLocation(FString& OutCmLocation)
{
	TArray<FString> Results;
	TArray<FString> ErrorMessages;
	const bool bResult = RunCommand(TEXT("location"), TArray<FString>(), TArray<FString>(), Results, ErrorMessages);
	if (bResult && Results.Num() > 0)
	{
		OutCmLocation = MoveTemp(Results[0]);
		return true;
	}
	return false;
}

bool GetConfigSetFilesAsReadOnly()
{
	TArray<FString> Results;
	TArray<FString> ErrorMessages;
	TArray<FString> Parameters;
	Parameters.Add(TEXT("setfileasreadonly"));
	const bool bResult = RunCommand(TEXT("getconfig"), Parameters, TArray<FString>(), Results, ErrorMessages);
	if (bResult && Results.Num() > 0)
	{
		if ((Results[0].Compare(TEXT("yes"), ESearchCase::IgnoreCase) == 0) || (Results[0].Compare(TEXT("true"), ESearchCase::IgnoreCase) == 0))
		{
			return true;
		}
	}
	return false;
}

void GetUserName(FString& OutUserName)
{
	TArray<FString> Results;
	TArray<FString> ErrorMessages;
	const bool bResult = RunCommand(TEXT("whoami"), TArray<FString>(), TArray<FString>(), Results, ErrorMessages);
	if (bResult && Results.Num() > 0)
	{
		OutUserName = MoveTemp(Results[0]);
	}
}

bool GetWorkspaceName(const FString& InWorkspaceRoot, FString& OutWorkspaceName, TArray<FString>& OutErrorMessages)
{
	TArray<FString> Results;
	TArray<FString> Parameters;
	Parameters.Add(TEXT("--format={wkname}"));
	TArray<FString> Files;
	Files.Add(InWorkspaceRoot); // Uses an absolute path so that the error message is explicit
	// Get the workspace name
	const bool bResult = RunCommand(TEXT("getworkspacefrompath"), Parameters, Files, Results, OutErrorMessages);
	if (bResult && Results.Num() > 0)
	{
		// NOTE: an old version of cm getworkspacefrompath didn't return an error code so we had to rely on the error message
		if (!Results[0].EndsWith(TEXT(" is not in a workspace.")))
		{
			OutWorkspaceName = MoveTemp(Results[0]);
		}
	}

	return bResult;
}

static bool ParseWorkspaceInfo(TArray<FString>& InResults, FString& OutBranchName, FString& OutRepositoryName, FString& OutServerUrl)
{
	if (InResults.Num() == 0)
	{
		return false;
	}

	// Get workspace information, in the form "Branch /main@UE5PlasticPluginDev@localhost:8087"
	//                                     or "Branch /main@UE5PlasticPluginDev@test@cloud" (when connected to the cloud)
	//                                     or "Branch /main@rep:UE5OpenWorldPerfTest@repserver:test@cloud"
	//                                     or "Changeset 1234@UE5PlasticPluginDev@test@cloud" (when the workspace is switched on a changeset instead of a branch)
	static const FString BranchPrefix(TEXT("Branch "));
	static const FString ChangesetPrefix(TEXT("Changeset "));
	static const FString LabelPrefix(TEXT("Label "));
	static const FString RepPrefix(TEXT("rep:"));
	static const FString RepserverPrefix(TEXT("repserver:"));
	FString& WorkspaceInfo = InResults[0];
	if (WorkspaceInfo.StartsWith(BranchPrefix, ESearchCase::CaseSensitive))
	{
		WorkspaceInfo.RightChopInline(BranchPrefix.Len());
	}
	else if (WorkspaceInfo.StartsWith(ChangesetPrefix, ESearchCase::CaseSensitive))
	{
		WorkspaceInfo.RightChopInline(ChangesetPrefix.Len());
	}
	else if (WorkspaceInfo.StartsWith(LabelPrefix, ESearchCase::CaseSensitive))
	{
		WorkspaceInfo.RightChopInline(LabelPrefix.Len());
	}
	else
	{
		return false;
	}

	TArray<FString> WorkspaceInfos;
	WorkspaceInfo.ParseIntoArray(WorkspaceInfos, TEXT("@"), false); // Don't cull empty values
	if (WorkspaceInfos.Num() >= 3)
	{
		OutBranchName = MoveTemp(WorkspaceInfos[0]);
		OutRepositoryName = MoveTemp(WorkspaceInfos[1]);
		OutServerUrl = MoveTemp(WorkspaceInfos[2]);

		if (OutRepositoryName.StartsWith(RepPrefix, ESearchCase::CaseSensitive))
		{
			OutRepositoryName.RightChopInline(RepPrefix.Len());
		}

		if (OutServerUrl.StartsWith(RepserverPrefix, ESearchCase::CaseSensitive))
		{
			OutServerUrl.RightChopInline(RepserverPrefix.Len());
		}

		if (WorkspaceInfos.Num() > 3) // (when connected to the cloud)
		{
			OutServerUrl.Append(TEXT("@"));
			OutServerUrl.Append(MoveTemp(WorkspaceInfos[3]));
		}
	}
	else
	{
		return false;
	}

	return true;
}

bool GetWorkspaceInfo(FString& OutBranchName, FString& OutRepositoryName, FString& OutServerUrl, TArray<FString>& OutErrorMessages)
{
	TArray<FString> Results;
	bool bResult = RunCommand(TEXT("workspaceinfo"), TArray<FString>(), TArray<FString>(), Results, OutErrorMessages);
	if (bResult)
	{
		bResult = ParseWorkspaceInfo(Results, OutBranchName, OutRepositoryName, OutServerUrl);
	}

	return bResult;
}

bool RunCheckConnection(FString& OutBranchName, FString& OutRepositoryName, FString& OutServerUrl, TArray<FString>& OutInfoMessages, TArray<FString>& OutErrorMessages)
{
	TArray<FString> Parameters;
	if (UnityVersionControlUtils::GetWorkspaceInfo(OutBranchName, OutRepositoryName, OutServerUrl, OutErrorMessages))
	{
		Parameters.Add(FString::Printf(TEXT("--server=%s"), *OutServerUrl));
	}
	return UnityVersionControlUtils::RunCommand(TEXT("checkconnection"), Parameters, TArray<FString>(), OutInfoMessages, OutErrorMessages);
}

FString UserNameToDisplayName(const FString& InUserName)
{
	if (const FString* Result = GetDefault<UUnityVersionControlProjectSettings>()->UserNameToDisplayName.Find(InUserName))
	{
		return *Result;
	}
	else if (GetDefault<UUnityVersionControlProjectSettings>()->bHideEmailDomainInUsername)
	{
		int32 EmailDomainSeparatorIndex;
		if (InUserName.FindChar(TEXT('@'), EmailDomainSeparatorIndex))
		{
			return InUserName.Left(EmailDomainSeparatorIndex);
		}
	}

	return InUserName;
}

/**
* Parse the current changeset from the header returned by "cm status --machinereadable --header --fieldseparator=;"
*
* Get workspace status in one of the form
STATUS;41;UEPlasticPluginDev;localhost:8087
STATUS;41;UEPlasticPluginDev;test@cloud
*
* @note The semicolon (";") that is used as filedseparator can also be used in the name of a repository.
*       This wouldn't be an issue with the current code, but we have to keep that in mind for future evolutions.
*/
static bool GetChangesetFromWorkspaceStatus(const TArray<FString>& InResults, int32& OutChangeset)
{
	if (InResults.Num() > 0)
	{
		const FString& WorkspaceStatus = InResults[0];
		TArray<FString> WorkspaceInfos;
		WorkspaceStatus.ParseIntoArray(WorkspaceInfos, FILE_STATUS_SEPARATOR, false); // Don't cull empty values in csv
		if (WorkspaceInfos.Num() >= 4)
		{
			OutChangeset = FCString::Atoi(*WorkspaceInfos[1]);
			return true;
		}
	}

	return false;
}

/**
 * Interpret the 2-to-8 letters file status from the given cm "status" result.
 *
 * @param InFileStatus The 2-to-8 letters file status from the given cm "status" result
 * @param bInUsesCheckedOutChanged If using the new --iscochanged "CO+CH"
 * @return EWorkspaceState
 *
 * @see #ParseFileStatusResult() for examples of results from "cm status --machinereadable"
*/
static EWorkspaceState StateFromStatus(const FString& InFileStatus, const bool bInUsesCheckedOutChanged)
{
	EWorkspaceState State;

	if (InFileStatus == "CH") // Modified but not Checked-Out
	{
		State = EWorkspaceState::Changed;
	}
	else if (InFileStatus == "CO") // Checked-Out with no change, or "don't know" if using on an old version of cm
	{
		// Recent version can distinguish between CheckedOut with or with no changes
		if (bInUsesCheckedOutChanged)
		{
			State = EWorkspaceState::CheckedOutUnchanged; // Recent version; here it's checkedout with no change
		}
		else
		{
			State = EWorkspaceState::CheckedOutChanged; // Older version; need to assume it is changed to retain behavior
		}

	}
	else if (InFileStatus == "CO+CH") // Checked-Out and changed from the new --iscochanged
	{
		State = EWorkspaceState::CheckedOutChanged; // Recent version; here it's checkedout with changes
	}
	else if (InFileStatus.Contains(TEXT("CP"))) // "CP", "CO+CP"
	{
		State = EWorkspaceState::Copied;
	}
	else if (InFileStatus.Contains(TEXT("MV"))) // "MV", "CO+MV", "CO+CH+MV", "CO+RP+MV"
	{
		State = EWorkspaceState::Moved; // Moved/Renamed
	}
	else if (InFileStatus.Contains(TEXT("RP"))) // "RP", "CO+RP", "CO+RP+CH", "CO+CH+RP"
	{
		State = EWorkspaceState::Replaced;
	}
	else if (InFileStatus == "AD")
	{
		State = EWorkspaceState::Added;
	}
	else if ((InFileStatus == "PR") || (InFileStatus == "LM")) // Not Controlled/Not in Depot/Untracked (or Locally Moved/Renamed)
	{
		State = EWorkspaceState::Private;
	}
	else if (InFileStatus == "IG")
	{
		State = EWorkspaceState::Ignored;
	}
	else if (InFileStatus == "DE")
	{
		State = EWorkspaceState::Deleted; // Deleted (removed from source control)
	}
	else if (InFileStatus == "LD")
	{
		State = EWorkspaceState::LocallyDeleted; // Locally Deleted (ie. missing)
	}
	else
	{
		UE_LOG(LogSourceControl, Warning, TEXT("Unknown file status '%s'"), *InFileStatus);
		State = EWorkspaceState::Unknown;
	}

	return State;
}

/**
 * Extract and interpret the file state from the cm "status" result.
 *
 * @param InResult One line of status from a "status" command
 * @param bInUsesCheckedOutChanged If using the new --iscochanged "CO+CH"
 * @return A workspace state
 *
 * Examples:
CO+CH;c:\Workspace\UEPlasticPluginDev\Content\Blueprints\CE_Game.uasset;False;NO_MERGES
MV;100%;c:\Workspace\UEPlasticPluginDev\Content\Blueprints\BP_ToRename.uasset;c:\Workspace\UEPlasticPluginDev\Content\Blueprints\BP_Renamed.uasset;False;NO_MERGES
 *
 * @see #ParseFileStatusResult() for more examples of results from "cm status --machinereadable"
*/
static FUnityVersionControlState StateFromStatusResult(const FString& InResult, const bool bInUsesCheckedOutChanged)
{
	TArray<FString> ResultElements;
	InResult.ParseIntoArray(ResultElements, FILE_STATUS_SEPARATOR, false); // Don't cull empty values in csv
	if (ResultElements.Num() >= 4) // Note: should contain 4 or 6 elements (for moved files)
	{
		EWorkspaceState WorkspaceState = StateFromStatus(ResultElements[0], bInUsesCheckedOutChanged);
		if (WorkspaceState == EWorkspaceState::Moved)
		{
			// Special case for an asset that has been moved/renamed
			FString& File = ResultElements[3];
			FUnityVersionControlState State(MoveTemp(File), WorkspaceState);
			State.MovedFrom = MoveTemp(ResultElements[2]);
			return State;
		}
		else
		{
			FString& File = ResultElements[1];
			return FUnityVersionControlState(MoveTemp(File), WorkspaceState);
		}
	}

	UE_LOG(LogSourceControl, Warning, TEXT("%s"), *InResult);

	return FUnityVersionControlState(FString());
}

/**
 * @brief Parse status results in case of a regular operation for a list of files (not for a whole directory).
 *
 * This is the most common scenario, for any operation from the Content Browser or the View Changes window.
 *
 * In this case, iterates on the list of files the Editor provides,
 * searching corresponding file status from the array of strings results of a "status" command.
 *
 * @param[in]	InFiles		List of files in a directory (never empty).
 * @param[in]	InResults	Lines of results from the "status" command
 * @param[out]	OutStates	States of files for witch the status has been gathered
 *
 * Example of results from "cm status --machinereadable"
CH;c:\Workspace\UEPlasticPluginDev\Content\Changed_BP.uasset;False;NO_MERGES
CO;c:\Workspace\UEPlasticPluginDev\Content\CheckedOutUnchanged_BP.uasset;False;NO_MERGES
CO+CH;c:\Workspace\UEPlasticPluginDev\Content\CheckedOutChanged_BP.uasset;False;NO_MERGES
CO+CP;c:\Workspace\UEPlasticPluginDev\Content\Copied_BP.uasset;False;NO_MERGES
CO+RP;c:\Workspace\UEPlasticPluginDev\Content\Replaced_BP.uasset;False;NO_MERGES
AD;c:\Workspace\UEPlasticPluginDev\Content\Added_BP.uasset;False;NO_MERGES
PR;c:\Workspace\UEPlasticPluginDev\Content\Private_BP.uasset;False;NO_MERGES
IG;c:\Workspace\UEPlasticPluginDev\Content\Ignored_BP.uasset;False;NO_MERGES
DE;c:\Workspace\UEPlasticPluginDev\Content\Deleted_BP.uasset;False;NO_MERGES
LD;c:\Workspace\UEPlasticPluginDev\Content\Deleted2_BP.uasset;False;NO_MERGES
MV;100%;c:\Workspace\UEPlasticPluginDev\Content\ToMove_BP.uasset;c:\Workspace\UEPlasticPluginDev\Content\Moved_BP.uasset
 *
 * @see #ParseDirectoryStatusResult() that use a different parse logic
 */
static void ParseFileStatusResult(TArray<FString>&& InFiles, const TArray<FString>& InResults, TArray<FUnityVersionControlState>& OutStates)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(UnityVersionControlUtils::ParseFileStatusResult);

	FUnityVersionControlProvider& Provider = FUnityVersionControlModule::Get().GetProvider();
	const FString& WorkspaceRoot = Provider.GetPathToWorkspaceRoot();
	const bool bUsesCheckedOutChanged = Provider.GetPlasticScmVersion() >= UnityVersionControlVersions::StatusIsCheckedOutChanged;

	// Parse the list of status results in a map indexed by absolute filename
	TMap<FString, FUnityVersionControlState> FileToStateMap;
	FileToStateMap.Reserve(InResults.Num());
	for (const FString& InResult : InResults)
	{
		FUnityVersionControlState State = StateFromStatusResult(InResult, bUsesCheckedOutChanged);
		FileToStateMap.Add(State.LocalFilename, MoveTemp(State));
	}

	// Iterate on each file explicitly listed in the command
	for (FString& InFile : InFiles)
	{
		FUnityVersionControlState FileState(MoveTemp(InFile));
		const FString& File = FileState.LocalFilename;

		// Search the file in the list of status
		if (FUnityVersionControlState* State = FileToStateMap.Find(File))
		{
			// File found in status results; only the case for "changed" (or checked-out) files
			FileState.WorkspaceState = State->WorkspaceState;

			// Extract the original name of a Moved/Renamed file
			if (EWorkspaceState::Moved == FileState.WorkspaceState)
			{
				FileState.MovedFrom = State->MovedFrom;
			}
		}
		else
		{
			// File not found in status
			if (FPaths::FileExists(File))
			{
				// usually means the file is unchanged, or is on Hidden changes
				FileState.WorkspaceState = EWorkspaceState::Controlled; // Unchanged
			}
			else
			{
				// but also the case for newly created content: there is no file on disk until the content is saved for the first time (but we cannot mark is as locally deleted)
				FileState.WorkspaceState = EWorkspaceState::Private; // Not Controlled
			}
		}

		// debug log (only for the first few files)
		if (OutStates.Num() < 20)
		{
			UE_LOG(LogSourceControl, Verbose, TEXT("%s = %d:%s"), *File, static_cast<uint32>(FileState.WorkspaceState), FileState.ToString());
		}

		OutStates.Add(MoveTemp(FileState));
	}
	// debug log (if too many files)
	if (OutStates.Num() > 20)
	{
		UE_LOG(LogSourceControl, Verbose, TEXT("[...] %d more files"), OutStates.Num() - 20);
	}
}

/**
 * @brief Parse file status in case of a "whole directory status" (no file listed in the command).
 *
 * This is a less common scenario, typically calling the Submit Content, Revert All or Refresh commands
 * from the global source control menu.
 *
 * In this case, as there is no file list to iterate over,
 * just parse each line of the array of strings results from the "status" command.
 *
 * @param[in]	InDir		The path to the directory (never empty).
 * @param[in]	InResults	Lines of results from the "status" command
 * @param[out]	OutStates	States of files for witch the status has been gathered
 *
 * @see #ParseFileStatusResult() above for an example of a results from "cm status --machinereadable"
*/
static void ParseDirectoryStatusResult(const FString& InDir, const TArray<FString>& InResults, TArray<FUnityVersionControlState>& OutStates)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(UnityVersionControlUtils::ParseDirectoryStatusResult);

	FUnityVersionControlProvider& Provider = FUnityVersionControlModule::Get().GetProvider();
	const bool bUsesCheckedOutChanged = Provider.GetPlasticScmVersion() >= UnityVersionControlVersions::StatusIsCheckedOutChanged;

	// First, find in the cache any existing states for files within the considered directory, that are not the default "Controlled" state
	TArray<FSourceControlStateRef> CachedStates = Provider.GetCachedStateByPredicate([&InDir](const FSourceControlStateRef& InState) {
		TSharedRef<FUnityVersionControlState, ESPMode::ThreadSafe> State = StaticCastSharedRef<FUnityVersionControlState>(InState);
		return (State->WorkspaceState != EWorkspaceState::Unknown) && (State->WorkspaceState != EWorkspaceState::Controlled) && InState->GetFilename().StartsWith(InDir);
	});

	// Iterate on each line of result of the status command
	for (const FString& InResult : InResults)
	{
		FUnityVersionControlState FileState = StateFromStatusResult(InResult, bUsesCheckedOutChanged);
		if (!FileState.LocalFilename.IsEmpty())
		{
			UE_LOG(LogSourceControl, Verbose, TEXT("%s = %d:%s"), *FileState.LocalFilename, static_cast<uint32>(FileState.WorkspaceState), FileState.ToString());

			// If a new state has been found in the directory status, we will update the cached state for the file later, let's remove it from the list
			CachedStates.RemoveAll([&CachedStates, &FileState](FSourceControlStateRef& PreviousState) {
				return PreviousState->GetFilename().Equals(FileState.GetFilename(), ESearchCase::IgnoreCase);
			});

			OutStates.Add(MoveTemp(FileState));
		}
	}

	// Finally, update the cache for the files that where not found in the status results (eg checked-in or reverted outside of the Editor)
	for (const auto& CachedState : CachedStates)
	{
		TSharedRef<FUnityVersionControlState, ESPMode::ThreadSafe> State = StaticCastSharedRef<FUnityVersionControlState>(CachedState);
		// Check if a file that was "deleted" or "locally deleted" has been reverted or checked-in by testing if it still exists on disk
		if (State->IsDeleted() && !FPaths::FileExists(State->GetFilename()))
		{
			// Remove the file from the cache if it has been deleted from disk
			Provider.RemoveFileFromCache(State->GetFilename());
		}
		else
		{
			// Switch back the file state to the default Controlled status (Unknown would prevent checkout)
			State->WorkspaceState = EWorkspaceState::Controlled;
		}

#if ENGINE_MAJOR_VERSION == 5
		// also remove the file from its changelist if any
		if (State->Changelist.IsInitialized())
		{
			// 1- Remove these files from their previous changelist
			TSharedRef<FUnityVersionControlChangelistState, ESPMode::ThreadSafe> ChangelistState = Provider.GetStateInternal(State->Changelist);
			ChangelistState->Files.Remove(State);
			// 2- And reset the reference to their previous changelist
			State->Changelist.Reset();
		}
#endif
	}
}

/// Visitor to list all files in subdirectory
class FFileVisitor : public IPlatformFile::FDirectoryVisitor
{
public:
	virtual bool Visit(const TCHAR* FilenameOrDirectory, bool bIsDirectory) override
	{
		if (!bIsDirectory)
		{
			Files.Add(FilenameOrDirectory);
		}
		return true;
	}

	TArray<FString> Files;
};

/**
 * @brief Run a "status" command for a directory to get the local workspace file states
 *
 *  ie. Changed, CheckedOut, Copied, Replaced, Added, Private, Ignored, Deleted, LocallyDeleted, Moved, LocallyMoved
 *
 *  It is either a command for a whole directory (ie. "Content/", in case of "Submit to Source Control"),
 * or for one or more files all on a same directory (by design, since we group files by directory in RunUpdateStatus())
 *
 * @param[in]	InDir				The path to the common directory of all the files listed after (never empty).
 * @param[in]	InFiles				List of files in a directory, or the path to the directory itself (never empty).
 * @param[in]	InSearchType		Call "status" with "--all", or with just "--controlledchanged" when doing only a quick check following a source control operation
 * @param[out]	OutErrorMessages	Error messages from the "status" command
 * @param[out]	OutStates			States of files for witch the status has been gathered (distinct than InFiles in case of a "directory status")
 * @param[out]	OutChangeset		The current Changeset Number
 * @param[out]	OutBranchName		Name of the current checked-out branch
 */
static bool RunStatus(const FString& InDir, TArray<FString>&& InFiles, const EStatusSearchType InSearchType, TArray<FString>& OutErrorMessages, TArray<FUnityVersionControlState>& OutStates, int32& OutChangeset, FString& OutBranchName)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(UnityVersionControlUtils::RunStatus);

	check(InFiles.Num() > 0);

	TArray<FString> Parameters;
	Parameters.Add(TEXT("--machinereadable"));
	Parameters.Add(TEXT("--fieldseparator=\"") FILE_STATUS_SEPARATOR TEXT("\""));
	if (InSearchType == EStatusSearchType::All)
	{
		// NOTE: don't use "--all" to avoid searching for --localmoved since it's the most time consuming (beside --changed)
		// and its not used by the plugin (matching similarities doesn't seem to work with .uasset files)
		Parameters.Add(TEXT("--controlledchanged"));
		// TODO: add a user settings to avoid searching for --changed and --localdeleted, to work like Perforce on big projects,
		// provided that the user understands the consequences (they won't see assets modified locally without a proper checkout)
		Parameters.Add(TEXT("--changed"));
		Parameters.Add(TEXT("--localdeleted"));
		Parameters.Add(TEXT("--private"));
		Parameters.Add(TEXT("--ignored"));

		// If the version of cm is recent enough use the new --iscochanged for "CO+CH" status
		const FUnityVersionControlProvider& Provider = FUnityVersionControlModule::Get().GetProvider();
		const bool bUsesCheckedOutChanged = Provider.GetPlasticScmVersion() >= UnityVersionControlVersions::StatusIsCheckedOutChanged;
		if (bUsesCheckedOutChanged)
		{
			Parameters.Add(TEXT("--iscochanged"));
		}
	}
	else if (InSearchType == EStatusSearchType::ControlledOnly)
	{
		Parameters.Add(TEXT("--controlledchanged"));
	}

	// "cm status" only operate on one path (file or directory) at a time, so use one common path for multiple files in a directory
	TArray<FString> OnePath;
	// Only one file: optim very useful for the .uproject file at the root to avoid parsing the whole repository
	// (but doesn't work if the file is deleted)
	const bool bSingleFile = (InFiles.Num() == 1) && (FPaths::FileExists(InFiles[0]));
	if (bSingleFile)
	{
		OnePath.Add(InFiles[0]);
	}
	else
	{
		OnePath.Add(InDir);
	}
	TArray<FString> Results;
	TArray<FString> ErrorMessages;
	const bool bResult = RunCommand(TEXT("status"), Parameters, OnePath, Results, ErrorMessages);
	OutErrorMessages.Append(MoveTemp(ErrorMessages));
	if (bResult)
	{
		// Parse the first line of status with the Changeset number, then remove it to work on a plain list of files
		if (Results.Num() > 0)
		{
			GetChangesetFromWorkspaceStatus(Results, OutChangeset);
			Results.RemoveAt(0, 1, false);
		}

		// Normalize file paths in the result (convert all '\' to '/')
		for (FString& Result : Results)
		{
			FPaths::NormalizeFilename(Result);
		}

		const bool bWholeDirectory = (InFiles.Num() == 1) && (InFiles[0] == InDir);
		if (bWholeDirectory)
		{
			// 1) Special case for "status" of a directory: requires a specific parse logic.
			//   (this is triggered by the "Submit to Source Control" top menu button, but also for the initial check, the global Revert etc)
			UE_LOG(LogSourceControl, Verbose, TEXT("RunStatus(%s): 1) special case for status of a directory:"), *InDir);
			ParseDirectoryStatusResult(InDir, Results, OutStates);
		}
		else
		{
			// 2) General case for one or more files in the same directory.
			UE_LOG(LogSourceControl, Verbose, TEXT("RunStatus(%s...): 2) general case for %d file(s) in a directory (%s)"), *InFiles[0], InFiles.Num(), *InDir);
			ParseFileStatusResult(MoveTemp(InFiles), Results, OutStates);
		}
	}

	return bResult;
}

// Parse the fileinfo output format "{RevisionChangeset};{RevisionHeadChangeset};{RepSpec};{LockedBy};{LockedWhere}"
// for example "40;41;repo@server:port;srombauts;UEPlasticPluginDev"
class FPlasticFileinfoParser
{
public:
	explicit FPlasticFileinfoParser(const FString& InResult)
	{
		TArray<FString> Fileinfos;
		InResult.ParseIntoArray(Fileinfos, TEXT(";"), false); // Don't cull empty values in csv
		if (Fileinfos.Num() == 5)
		{
			RevisionChangeset = FCString::Atoi(*Fileinfos[0]);
			RevisionHeadChangeset = FCString::Atoi(*Fileinfos[1]);
			RepSpec = MoveTemp(Fileinfos[2]);
			LockedBy = UserNameToDisplayName(MoveTemp(Fileinfos[3]));
			LockedWhere = MoveTemp(Fileinfos[4]);
		}
	}

	int32 RevisionChangeset;
	int32 RevisionHeadChangeset;
	FString RepSpec;
	FString LockedBy;
	FString LockedWhere;
};

/** Parse the array of strings result of a 'cm fileinfo --format="{RevisionChangeset};{RevisionHeadChangeset};{RepSpec};{LockedBy};{LockedWhere}"' command
 *
 * Example cm fileinfo results:
16;16;;
14;15;;
17;17;srombauts;Workspace_2
 */
static void ParseFileinfoResults(const TArray<FString>& InResults, TArray<FUnityVersionControlState>& InOutStates)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(UnityVersionControlUtils::ParseFileinfoResults);

	ensureMsgf(InResults.Num() == InOutStates.Num(), TEXT("The fileinfo command should gives the same number of infos as the status command"));

	// Iterate on all files and all status of the result (assuming same number of line of results than number of file states)
	for (int32 IdxResult = 0; IdxResult < InResults.Num(); IdxResult++)
	{
		const FString& Fileinfo = InResults[IdxResult];
		FUnityVersionControlState& FileState = InOutStates[IdxResult];
		const FString& File = FileState.LocalFilename;
		FPlasticFileinfoParser FileinfoParser(Fileinfo);

		FileState.LocalRevisionChangeset = FileinfoParser.RevisionChangeset;
		FileState.DepotRevisionChangeset = FileinfoParser.RevisionHeadChangeset;
		FileState.RepSpec = FileinfoParser.RepSpec;
		FileState.LockedBy = MoveTemp(FileinfoParser.LockedBy);
		FileState.LockedWhere = MoveTemp(FileinfoParser.LockedWhere);

		// debug log (only for the first few files)
		if (IdxResult < 20)
		{
			UE_LOG(LogSourceControl, Verbose, TEXT("%s: %d;%d %s by '%s' (%s)"), *File, FileState.LocalRevisionChangeset, FileState.DepotRevisionChangeset, *FileState.RepSpec, *FileState.LockedBy, *FileState.LockedWhere);
		}
	}
	// debug log (if too many files)
	if (InResults.Num() > 20)
	{
		UE_LOG(LogSourceControl, Verbose, TEXT("[...] %d more files"), InResults.Num() - 20);
	}
}

/**
 * @brief Run a "fileinfo" command to update complementary status information of given files.
 *
 * ie RevisionChangeset, RevisionHeadChangeset, RepSpec, LockedBy, LockedWhere
 *
 * @param[in]		bInWholeDirectory	If executed on a whole directory (typically Content/) for a "Submit Content" operation, optimize fileinfo more aggressively
 * @param			bInUpdateHistory	If getting the history of files, force execute the fileinfo command required to get RepSpec of XLinks (history view or visual diff)
 * @param[out]		OutErrorMessages	Error messages from the "fileinfo" command
 * @param[in,out]	InOutStates			List of file states in the directory, gathered by the "status" command, completed by results of the "fileinfo" command
 */
static bool RunFileinfo(const bool bInWholeDirectory, const bool bInUpdateHistory, TArray<FString>& OutErrorMessages, TArray<FUnityVersionControlState>& InOutStates)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(UnityVersionControlUtils::RunFileinfo);

	bool bResult = true;
	TArray<FString> SelectedFiles;

	TArray<FUnityVersionControlState> SelectedStates;
	TArray<FUnityVersionControlState> OptimizedStates;
	for (FUnityVersionControlState& State : InOutStates)
	{
		// 1) Issue a "fileinfo" command for controlled files (to know if they are up to date and can be checked-out or checked-in)
		// but only if controlled unchanged, or locally changed / locally deleted,
		// optimizing for files that are CheckedOut/Added/Deleted/Moved/Copied/Replaced/NotControled/Ignored/Private/Unknown
		// (since there is no point to check if they are up to date in these cases; they are already checked-out or not controlled).
		// This greatly reduce the time needed to do some operations like "Add" or "Move/Rename/Copy" when there is some latency with the server (eg cloud).
		//
		// 2) bInWholeDirectory: In the case of a "whole directory status" triggered by the "Submit Content" operation,
		// don't even issue a "fileinfo" command for unchanged Controlled files since they won't be considered them for submit.
		// This greatly reduce the time needed to open the Submit window.
		//
		// 3) bInUpdateHistory: When the plugin needs to update the history of files, it needs to know if it's on a XLink,
		// so the fileinfo command is required here to get the RepSpec
		if (bInUpdateHistory
			|| ((State.WorkspaceState == EWorkspaceState::Controlled) && !bInWholeDirectory)
			||	(State.WorkspaceState == EWorkspaceState::Changed)
			||	(State.WorkspaceState == EWorkspaceState::LocallyDeleted)
			)
		{
			SelectedFiles.Add(State.LocalFilename);
			SelectedStates.Add(MoveTemp(State));
		}
		else
		{
			OptimizedStates.Add(MoveTemp(State));
		}
	}
	InOutStates = MoveTemp(OptimizedStates);

	if (SelectedStates.Num())
	{
		TArray<FString> Results;
		TArray<FString> ErrorMessages;
		TArray<FString> Parameters;
		Parameters.Add(TEXT("--format=\"{RevisionChangeset};{RevisionHeadChangeset};{RepSpec};{LockedBy};{LockedWhere}\""));
		bResult = RunCommand(TEXT("fileinfo"), Parameters, SelectedFiles, Results, ErrorMessages);
		OutErrorMessages.Append(MoveTemp(ErrorMessages));
		if (bResult)
		{
			ParseFileinfoResults(Results, SelectedStates);
			InOutStates.Append(MoveTemp(SelectedStates));
		}
	}

	return bResult;
}

// FILE_CONFLICT /Content/FirstPersonBP/Blueprints/FirstPersonProjectile.uasset 1 4 6 903
// (explanations: 'The file /Content/FirstPersonBP/Blueprints/FirstPersonProjectile.uasset needs to be merged from cs:4 to cs:6 base cs:1. Changed by both contributors.')
class FPlasticMergeConflictParser
{
public:
	explicit FPlasticMergeConflictParser(const FString& InResult)
	{
		static const FString FILE_CONFLICT(TEXT("FILE_CONFLICT "));
		if (InResult.StartsWith(FILE_CONFLICT, ESearchCase::CaseSensitive))
		{
			FString Temp = InResult.RightChop(FILE_CONFLICT.Len());
			int32 WhitespaceIndex;
			if (Temp.FindChar(TEXT(' '), WhitespaceIndex))
			{
				Filename = Temp.Left(WhitespaceIndex);
			}
			Temp.RightChopInline(WhitespaceIndex + 1);
			if (Temp.FindChar(TEXT(' '), WhitespaceIndex))
			{
				BaseChangeset = Temp.Left(WhitespaceIndex);
			}
			Temp.RightChopInline(WhitespaceIndex + 1);
			if (Temp.FindChar(TEXT(' '), WhitespaceIndex))
			{
				SourceChangeset = Temp.Left(WhitespaceIndex);
			}
		}
	}

	FString Filename;
	FString BaseChangeset;
	FString SourceChangeset;
};

// Check if merging, and from which changelist, then execute a cm merge command to amend status for listed files
static bool RunCheckMergeStatus(const TArray<FString>& InFiles, TArray<FString>& OutErrorMessages, TArray<FUnityVersionControlState>& OutStates)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(UnityVersionControlUtils::RunCheckMergeStatus);

	bool bResult = false;
	const FUnityVersionControlProvider& Provider = FUnityVersionControlModule::Get().GetProvider();

	const FString MergeProgressFilename = FPaths::Combine(*Provider.GetPathToWorkspaceRoot(), TEXT(".plastic/plastic.mergeprogress"));
	if (FPaths::FileExists(MergeProgressFilename))
	{
		// read in file as string
		FString MergeProgressContent;
		if (FFileHelper::LoadFileToString(MergeProgressContent, *MergeProgressFilename))
		{
			UE_LOG(LogSourceControl, Verbose, TEXT("RunCheckMergeStatus: %s:\n%s"), *MergeProgressFilename, *MergeProgressContent);
			// Content is in one line, looking like the following:
			// Target: mount:56e62dd7-241f-41e9-8c6b-dd4ca4513e62#/#UEMergeTest@localhost:8087 merged from: Merge 4
			// Target: mount:56e62dd7-241f-41e9-8c6b-dd4ca4513e62#/#UEMergeTest@localhost:8087 merged from: Cherrypicking 3
			// Target: mount:56e62dd7-241f-41e9-8c6b-dd4ca4513e62#/#UEMergeTest@localhost:8087 merged from: IntervalCherrypick 2 4
			// 1) Extract the word after "merged from: "
			static const FString MergeFromString(TEXT("merged from: "));
			const int32 MergeFromIndex = MergeProgressContent.Find(MergeFromString, ESearchCase::CaseSensitive);
			if (MergeFromIndex > INDEX_NONE)
			{
				const FString MergeType = MergeProgressContent.RightChop(MergeFromIndex + MergeFromString.Len());
				int32 SpaceBeforeChangesetIndex;
				if (MergeType.FindChar(TEXT(' '), SpaceBeforeChangesetIndex))
				{
					// 2) In case of "Merge" or "Cherrypicking" extract the merge changelist xxx after the last space (use case for merge from "branch", from "label", and for "merge on Update")
					const FString ChangesetString = MergeType.RightChop(SpaceBeforeChangesetIndex + 1);
					const int32 Changeset = FCString::Atoi(*ChangesetString);
					const FString ChangesetSpecification = FString::Printf(TEXT("cs:%d"), Changeset);

					TArray<FString> Results;
					TArray<FString> ErrorMessages;
					TArray<FString> Parameters;
					Parameters.Add(ChangesetSpecification);

					int32 SpaceBeforeChangeset2Index;
					if (ChangesetString.FindLastChar(TEXT(' '), SpaceBeforeChangeset2Index))
					{
						// 3) In case of "IntervalCherrypick", extract the 2 changelists
						const FString Changeset2String = ChangesetString.RightChop(SpaceBeforeChangeset2Index + 1);
						const int32 Changeset2 = FCString::Atoi(*Changeset2String);
						const FString Changeset2Specification = FString::Printf(TEXT("--interval-origin=cs:%d"), Changeset2);

						Parameters.Add(Changeset2Specification);
					}
					else
					{
						if (MergeType.StartsWith(TEXT("Cherrypicking"), ESearchCase::CaseSensitive))
						{
							Parameters.Add(TEXT("--cherrypicking"));
						}
					}
					// Store the Merge Parameters for reuse with later "Resolve" operation
					const TArray<FString> PendingMergeParameters = Parameters;
					Parameters.Add(TEXT("--machinereadable"));
					// call 'cm merge cs:xxx --machinereadable' (only dry-run, without the --merge parameter)
					bResult = RunCommand(TEXT("merge"), Parameters, TArray<FString>(), Results, ErrorMessages);
					OutErrorMessages.Append(MoveTemp(ErrorMessages));
					// Parse the result, one line for each conflicted files:
					for (const FString& Result : Results)
					{
						FPlasticMergeConflictParser MergeConflict(Result);
						UE_LOG(LogSourceControl, Log, TEXT("MergeConflict.Filename: '%s'"), *MergeConflict.Filename);
						for (FUnityVersionControlState& State : OutStates)
						{
							UE_LOG(LogSourceControl, Log, TEXT("State.LocalFilename: '%s'"), *State.LocalFilename);
							if (State.LocalFilename.EndsWith(MergeConflict.Filename, ESearchCase::CaseSensitive))
							{
								UE_LOG(LogSourceControl, Verbose, TEXT("MergeConflict '%s' found Base cs:%s From cs:%s"), *MergeConflict.Filename, *MergeConflict.BaseChangeset, *MergeConflict.SourceChangeset);
								State.WorkspaceState = EWorkspaceState::Conflicted;
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3
								State.PendingResolveInfo = {
									MergeConflict.Filename,
									MergeConflict.Filename,
									MergeConflict.SourceChangeset,
									MergeConflict.BaseChangeset
								};
#else
								State.PendingMergeFilename = MergeConflict.Filename;
								State.PendingMergeBaseChangeset = FCString::Atoi(*MergeConflict.BaseChangeset);
								State.PendingMergeSourceChangeset = FCString::Atoi(*MergeConflict.SourceChangeset);
#endif
								State.PendingMergeParameters = PendingMergeParameters;
								break;
							}
						}
					}
				}
			}
		}
	}

	return bResult;
}

FString FindCommonDirectory(const FString& InPath1, const FString& InPath2)
{
	const int32 MinLen = FMath::Min(InPath1.Len(), InPath2.Len());
	int32 IndexAfterLastCommonSeparator = 0;
	for (int32 Index = 0; Index < MinLen; Index++)
	{
		if (InPath1[Index] != InPath2[Index])
		{
			break;
		}
		if (InPath1[Index] == TEXT('/'))
		{
			IndexAfterLastCommonSeparator = Index + 1;
		}
	}
	return InPath1.Left(IndexAfterLastCommonSeparator);
}

// Structure to group all files belonging to a root dir, storing their best/longest common directory
struct FFilesInCommonDir
{
	// Best/longest common directory, slash terminated, based on FindCommonDirectory()
	FString			CommonDir;
	TArray<FString>	Files;
};

// Run a batch of Plastic "status" and "fileinfo" commands to update status of given files and directories.
bool RunUpdateStatus(const TArray<FString>& InFiles, const EStatusSearchType InSearchType, const bool bInUpdateHistory, TArray<FString>& OutErrorMessages, TArray<FUnityVersionControlState>& OutStates, int32& OutChangeset, FString& OutBranchName)
{
	bool bResults = true;

	const FString& WorkspaceRoot = FUnityVersionControlModule::Get().GetProvider().GetPathToWorkspaceRoot();

	// The "status" command only operate on one directory-tree at a time (whole tree recursively)
	// not on different folders with no common root.
	// But "Submit to Source Control" ask for the State of many different directories,
	// from Project/Content and Project/Config, Engine/Content, Engine/Plugins/<...>/Content...

	// In a similar way, a checkin can involve files from different subdirectories, and UpdateStatus is called for all of them at once.

	static TArray<FString> RootDirs =
	{
		FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir()),
		FPaths::ConvertRelativePathToFull(FPaths::ProjectConfigDir()),
		FPaths::ConvertRelativePathToFull(FPaths::ProjectPluginsDir()),
		FPaths::ConvertRelativePathToFull(FPaths::GameSourceDir()),
		FPaths::ConvertRelativePathToFull(FPaths::EngineContentDir())
	};

	// 1) So here we group files by path (ie. by subdirectory)
	TMap<FString, FFilesInCommonDir> GroupOfFiles;
	for (const FString& File : InFiles)
	{
		// Discard all file/paths that are not under the workspace root (typically excluding the Engine content)
		if (!File.StartsWith(WorkspaceRoot))
		{
			UE_LOG(LogSourceControl, Verbose, TEXT("%s is out of the Workspace"), *File);
			continue;
		}

		bool bDirFound = false;
		for (const auto& RootDir : RootDirs)
		{
			if (File.StartsWith(RootDir))
			{
				FFilesInCommonDir* ExistingGroup = GroupOfFiles.Find(RootDir);
				if (ExistingGroup != nullptr)
				{
					// See if we have to update the CommonDir
					if (!File.StartsWith(ExistingGroup->CommonDir))
					{
						// the file is not in the same path, we need to find their common dir
						ExistingGroup->CommonDir = FindCommonDirectory(ExistingGroup->CommonDir, File);
					}
					ExistingGroup->Files.Add(File);
				}
				else
				{
					FString Path = FPaths::GetPath(File) + TEXT('/');
					GroupOfFiles.Add(RootDir, { MoveTemp(Path), {File}});
				}

				bDirFound = true;
				break;
			}
		}

		// If the file isn't part of our root directories, we simply add its directory as a new group.
		// It means that the group is dedicated to the directory, and as such its CommonDir is the directory itself.
		// This should be an edge case (typically the uproject file) .
		if (!bDirFound)
		{
			FString Path = FPaths::GetPath(File) + TEXT('/');
			FFilesInCommonDir* ExistingGroup = GroupOfFiles.Find(Path);
			if (ExistingGroup != nullptr)
			{
				ExistingGroup->Files.Add(File);
			}
			else
			{
				GroupOfFiles.Add(Path, { MoveTemp(Path), {File} });
			}
		}
	}

	if (InFiles.Num() > 0)
	{
		UE_LOG(LogSourceControl, Verbose, TEXT("RunUpdateStatus: %d file(s)/%d directory(ies) ('%s'...)"), InFiles.Num(), GroupOfFiles.Num(), *InFiles[0]);
	}
	else
	{
		UE_LOG(LogSourceControl, Warning, TEXT("RunUpdateStatus: NO file"));
	}

	// 2) then we can batch Plastic status operation by subdirectory
	for (auto& Group : GroupOfFiles)
	{
		const bool bWholeDirectory = ((Group.Value.Files.Num() == 1) && (Group.Value.CommonDir == Group.Value.Files[0]));

		// Run a "status" command on the directory to get workspace file states.
		// (ie. Changed, CheckedOut, Copied, Replaced, Added, Private, Ignored, Deleted, LocallyDeleted, Moved, LocallyMoved)
		TArray<FUnityVersionControlState> States;
		const bool bGroupOk = RunStatus(Group.Value.CommonDir, MoveTemp(Group.Value.Files), InSearchType, OutErrorMessages, States, OutChangeset, OutBranchName);
		if (!bGroupOk)
		{
			bResults = false;
		}
		else if (States.Num() > 0)
		{
			// Run a "fileinfo" command to update complementary status information of given files.
			// (ie RevisionChangeset, RevisionHeadChangeset, RepSpec, LockedBy, LockedWhere)
			// In case of "whole directory status", there is no explicit file in the group (it contains only the directory)
			// => work on the list of files discovered by RunStatus()
			bResults &= RunFileinfo(bWholeDirectory, bInUpdateHistory, OutErrorMessages, States);
		}
		OutStates.Append(MoveTemp(States));
	}

	// Check if merging, and from which changelist, then execute a cm merge command to amend status for listed files
	RunCheckMergeStatus(InFiles, OutErrorMessages, OutStates);

	return bResults;
}

// Run a "getfile" command to dump the binary content of a revision into a file.
bool RunGetFile(const FString& InRevSpec, const FString& InDumpFileName)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(UnityVersionControlUtils::RunGetFile);

	int32	ReturnCode = 0;
	FString Results;
	FString Errors;

	TArray<FString> Parameters;
	Parameters.Add(FString::Printf(TEXT("\"%s\""), *InRevSpec));
	Parameters.Add(TEXT("--raw"));
	Parameters.Add(FString::Printf(TEXT("--file=\"%s\""), *InDumpFileName));
	const bool bResult = UnityVersionControlUtils::RunCommand(TEXT("getfile"), Parameters, TArray<FString>(), Results, Errors);

	return bResult;
}

// Convert a file state to a string ala Perforce, see also ParseShelveFileStatus()
FString FileStateToAction(const EWorkspaceState InState)
{
	switch (InState)
	{
	case EWorkspaceState::Added:
		return TEXT("add");
	case EWorkspaceState::Deleted:
		return TEXT("delete");
	case EWorkspaceState::Moved:
		return TEXT("branch");
	case EWorkspaceState::CheckedOutChanged:
	default:
		return TEXT("edit");
	}
}

// TODO PR to move this in Engine
FString DecodeXmlEntities(const FString& InString)
{
	FString String = InString;
	String.ReplaceInline(TEXT("&amp;"), TEXT("&"));
	String.ReplaceInline(TEXT("&quot;"), TEXT("\""));
	String.ReplaceInline(TEXT("&apos;"), TEXT("'"));
	String.ReplaceInline(TEXT("&lt;"), TEXT("<"));
	String.ReplaceInline(TEXT("&gt;"), TEXT(">"));
	return String;
}

/**
 * Parse results of the 'cm history --moveddeleted --xml --encoding="utf-8"' command.
 *
 * Results of the history command looks like that:
<RevisionHistoriesResult>
  <RevisionHistories>
	<RevisionHistory>
	  <ItemName>C:/Workspace/UE4PlasticPluginDev/Content/FirstPersonBP/Blueprints/BP_TestsRenamed.uasset</ItemName>
	  <Revisions>
		<Revision>
		  <RevisionSpec>C:/Workspace/UE4PlasticPluginDev/Content/FirstPersonBP/Blueprints/BP_TestsRenamed.uasset#cs:7</RevisionSpec>
		  <Branch>/main</Branch>
		  <CreationDate>2019-10-14T09:52:07+02:00</CreationDate>
		  <RevisionType>bin</RevisionType>
		  <ChangesetNumber>7</ChangesetNumber>
		  <Owner>SRombauts</Owner>
		  <Comment>New tests</Comment>
		  <Repository>UE4PlasticPluginDev</Repository>
		  <Server>localhost:8087</Server>
		  <RepositorySpec>UE4PlasticPluginDev@localhost:8087</RepositorySpec>
		  <Size>22356</Size>
		  <Hash>zzuB6G9fbWz1md12+tvBxg==</Hash>
		</Revision>
		...
		<Revision>
		  <RevisionSpec>C:/Workspace/UE4PlasticPluginDev/Content/FirstPersonBP/Blueprints/BP_TestsRenamed.uasset#cs:12</RevisionSpec>
		  <Branch>Removed /Content/FirstPersonBP/Blueprints/BP_TestsRenamed.uasset</Branch>
		  <CreationDate>2022-04-28T16:00:37+02:00</CreationDate>
		  <RevisionType />
		  <ChangesetNumber>12</ChangesetNumber>
		  <Owner>sebastien.rombauts</Owner>
		  <Comment />
		  <Repository>UE4PlasticPluginDev</Repository>
		  <Server>localhost:8087</Server>
		  <RepositorySpec>UE4PlasticPluginDev@localhost:8087</RepositorySpec>
		  <Size>22406</Size>
		  <Hash>uR7NdDRAyKqADdyAqh67Rg==</Hash>
		</Revision>

	  </Revisions>
	</RevisionHistory>
	<RevisionHistory>
	  <ItemName>C:/Workspace/UE4PlasticPluginDev/Content/FirstPersonBP/Blueprints/BP_YetAnother.uasset</ItemName>
		...
	</RevisionHistory>
  </RevisionHistories>
</RevisionHistoriesResult>
*/
static bool ParseHistoryResults(const bool bInUpdateHistory, const FXmlFile& InXmlResult, TArray<FUnityVersionControlState>& InOutStates)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(UnityVersionControlUtils::ParseHistoryResults);

	const FUnityVersionControlProvider& Provider = FUnityVersionControlModule::Get().GetProvider();
	const FString RootRepSpec = FString::Printf(TEXT("%s@%s"), *Provider.GetRepositoryName(), *Provider.GetServerUrl());

	static const FString RevisionHistoriesResult(TEXT("RevisionHistoriesResult"));
	static const FString RevisionHistories(TEXT("RevisionHistories"));
	static const FString RevisionHistory(TEXT("RevisionHistory"));
	static const FString ItemName(TEXT("ItemName"));
	static const FString Revisions(TEXT("Revisions"));
	static const FString Revision(TEXT("Revision"));
	static const FString Branch(TEXT("Branch"));
	static const FString CreationDate(TEXT("CreationDate"));
	static const FString RevisionType(TEXT("RevisionType"));
	static const FString ChangesetNumber(TEXT("ChangesetNumber"));
	static const FString Owner(TEXT("Owner"));
	static const FString Comment(TEXT("Comment"));
	static const FString Size(TEXT("Size"));
	static const FString Hash(TEXT("Hash"));

	const FXmlNode* RevisionHistoriesResultNode = InXmlResult.GetRootNode();
	if (RevisionHistoriesResultNode == nullptr || RevisionHistoriesResultNode->GetTag() != RevisionHistoriesResult)
	{
		return false;
	}

	const FXmlNode* RevisionHistoriesNode = RevisionHistoriesResultNode->FindChildNode(RevisionHistories);
	if (RevisionHistoriesNode == nullptr)
	{
		return false;
	}

	const TArray<FXmlNode*>& RevisionHistoryNodes = RevisionHistoriesNode->GetChildrenNodes();
	for (const FXmlNode* RevisionHistoryNode : RevisionHistoryNodes)
	{
		const FXmlNode* ItemNameNode = RevisionHistoryNode->FindChildNode(ItemName);
		if (ItemNameNode == nullptr)
		{
			continue;
		}

		const FString Filename = ItemNameNode->GetContent();
		FUnityVersionControlState* InOutStatePtr = InOutStates.FindByPredicate(
			[&Filename](const FUnityVersionControlState& State) { return State.LocalFilename == Filename; }
		); // NOLINT(whitespace/parens) "Closing ) should be moved to the previous line" doesn't work well for lambda functions
		if (InOutStatePtr == nullptr)
		{
			continue;
		}
		FUnityVersionControlState& InOutState = *InOutStatePtr;

		const FXmlNode* RevisionsNode = RevisionHistoryNode->FindChildNode(Revisions);
		if (RevisionsNode == nullptr)
		{
			continue;
		}

		const TArray<FXmlNode*>& RevisionNodes = RevisionsNode->GetChildrenNodes();
		if (bInUpdateHistory)
		{
			InOutState.History.Reserve(RevisionNodes.Num());
		}

		// parse history in reverse: needed to get most recent at the top (implied by the UI)
		// Note: limit to last 100 changes, like Perforce
		static const int32 MaxRevisions = 100;
		const int32 MinIndex = FMath::Max(0, RevisionNodes.Num() - MaxRevisions);
		for (int32 Index = RevisionNodes.Num() - 1; Index >= MinIndex; Index--)
		{
			if (const FXmlNode* RevisionNode = RevisionNodes[Index])
			{
#if ENGINE_MAJOR_VERSION == 4
				const TSharedRef<FUnityVersionControlRevision, ESPMode::ThreadSafe> SourceControlRevision = MakeShareable(new FUnityVersionControlRevision);
#elif ENGINE_MAJOR_VERSION == 5
				const TSharedRef<FUnityVersionControlRevision, ESPMode::ThreadSafe> SourceControlRevision = MakeShared<FUnityVersionControlRevision>();
#endif
				SourceControlRevision->State = &InOutState;
				SourceControlRevision->Filename = Filename;

				if (const FXmlNode* RevisionTypeNode = RevisionNode->FindChildNode(RevisionType))
				{
					if (!RevisionTypeNode->GetContent().IsEmpty())
					{
						if (Index == 0)
						{
							SourceControlRevision->Action = FileStateToAction(EWorkspaceState::Added);
						}
						else
						{
							SourceControlRevision->Action = FileStateToAction(EWorkspaceState::CheckedOutChanged);
						}
					}
					else
					{
						SourceControlRevision->Action = FileStateToAction(EWorkspaceState::Deleted);
					}
				}

				if (const FXmlNode* ChangesetNumberNode = RevisionNode->FindChildNode(ChangesetNumber))
				{
					const FString& Changeset = ChangesetNumberNode->GetContent();
					SourceControlRevision->ChangesetNumber = FCString::Atoi(*Changeset); // Value now used in the Revision column and in the Asset Menu History

					// Also append depot name to the revision, but only when it is different from the default one (ie for xlinks sub repository)
					if (!InOutState.RepSpec.IsEmpty() && (InOutState.RepSpec != RootRepSpec))
					{
						TArray<FString> RepSpecs;
						InOutState.RepSpec.ParseIntoArray(RepSpecs, TEXT("@"));
						SourceControlRevision->Revision = FString::Printf(TEXT("cs:%s@%s"), *Changeset, *RepSpecs[0]);
					}
					else
					{
						SourceControlRevision->Revision = FString::Printf(TEXT("cs:%s"), *Changeset);
					}
				}
				if (const FXmlNode* CommentNode = RevisionNode->FindChildNode(Comment))
				{
					SourceControlRevision->Description = DecodeXmlEntities(CommentNode->GetContent());
				}
				if (const FXmlNode* OwnerNode = RevisionNode->FindChildNode(Owner))
				{
					SourceControlRevision->UserName = UserNameToDisplayName(OwnerNode->GetContent());
				}
				if (const FXmlNode* DateNode = RevisionNode->FindChildNode(CreationDate))
				{
					FString DateIso = DateNode->GetContent();
					const int len = DateIso.Len();
					if (DateIso.Len() > 29)
					{	//                           |--|
						//    2016-04-18T10:44:49.0000000+02:00
						// => 2016-04-18T10:44:49.000+02:00
						DateIso = DateNode->GetContent().LeftChop(10) + DateNode->GetContent().RightChop(27);
					}
					FDateTime::ParseIso8601(*DateIso, SourceControlRevision->Date);
				}
				if (const FXmlNode* BranchNode = RevisionNode->FindChildNode(Branch))
				{
					SourceControlRevision->Branch = BranchNode->GetContent();
				}
				if (const FXmlNode* SizeNode = RevisionNode->FindChildNode(Size))
				{
					SourceControlRevision->FileSize = FCString::Atoi(*SizeNode->GetContent());
				}

				// A negative RevisionHeadChangeset provided by fileinfo mean that the file has been unshelved;
				// replace it by the changeset number of the first revision in the history (the more recent)
				// Note: workaround to be able to show the history / the diff of a file that has been unshelved
				// (but keeps the LocalRevisionChangeset to the negative changeset corresponding to the Shelve Id)
				if (InOutState.DepotRevisionChangeset < 0)
				{
					InOutState.DepotRevisionChangeset = SourceControlRevision->ChangesetNumber;
				}

				// Detect and skip more recent changesets on other branches (ie above the RevisionHeadChangeset)
				// since we usually don't want to display changes from other branches in the History window...
				// except in case of a merge conflict, where the Editor expects the tip of the "source (remote)" branch to be at the top of the history!
				if (   (SourceControlRevision->ChangesetNumber > InOutState.DepotRevisionChangeset)
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3
					&& (SourceControlRevision->GetRevision() != InOutState.PendingResolveInfo.RemoteRevision))
#else
					&& (SourceControlRevision->ChangesetNumber != InOutState.PendingMergeSourceChangeset))
#endif
				{
					InOutState.HeadBranch = SourceControlRevision->Branch;
					InOutState.HeadAction = SourceControlRevision->Action;
					InOutState.HeadChangeList = SourceControlRevision->ChangesetNumber;
					InOutState.HeadUserName = SourceControlRevision->UserName;
					InOutState.HeadModTime = SourceControlRevision->Date.ToUnixTimestamp();
				}
				else if (bInUpdateHistory)
				{
					InOutState.History.Add(SourceControlRevision);
				}

				// Also grab the UserName of the author of the current depot/head changeset
				if ((SourceControlRevision->ChangesetNumber == InOutState.DepotRevisionChangeset) && InOutState.HeadUserName.IsEmpty())
				{
					InOutState.HeadUserName = SourceControlRevision->UserName;
				}

				if (!bInUpdateHistory)
				{
					break; // if not updating the history, just getting the head of the latest branch is enough
				}
			}
		}
	}

	return true;
}

// Run a Plastic "history" command and parse it's XML result.
bool RunGetHistory(const bool bInUpdateHistory, TArray<FUnityVersionControlState>& InOutStates, TArray<FString>& OutErrorMessages)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(UnityVersionControlUtils::RunGetHistory);

	bool bResult = true;
	FString Results;
	FString Errors;
	TArray<FString> Parameters;
	// Detecting move and deletion is costly as it is implemented as two extra queries to the server; do it only when getting the history of the current branch
	if (bInUpdateHistory)
	{
		Parameters.Add(TEXT("--moveddeleted"));
	}
	Parameters.Add(TEXT("--xml"));
	Parameters.Add(TEXT("--encoding=\"utf-8\""));
	const FUnityVersionControlProvider& Provider = FUnityVersionControlModule::Get().GetProvider();
	if (Provider.GetPlasticScmVersion() >= UnityVersionControlVersions::NewHistoryLimit)
	{
		if (bInUpdateHistory)
		{
			// --limit=0 will not limit the number of revisions, as stated by LimitNumberOfRevisionsInHistory
			Parameters.Add(FString::Printf(TEXT("--limit=%d"), GetDefault<UUnityVersionControlProjectSettings>()->LimitNumberOfRevisionsInHistory));
		}
		else
		{
			// when only searching for more recent changes on other branches, only the last revision is needed (to compare to the head of the current branch)
			Parameters.Add(TEXT("--limit=1"));
		}
	}

	TArray<FString> Files;
	Files.Reserve(InOutStates.Num());
	for (const FUnityVersionControlState& State : InOutStates)
	{
		// When getting only the last revision, optimize out if DepotRevisionChangeset is invalid (ie "fileinfo" was optimized out, eg for checked-out files)
		if (!bInUpdateHistory && State.DepotRevisionChangeset == ISourceControlState::INVALID_REVISION)
			continue;

		if (State.IsSourceControlled() && !State.IsAdded())
		{
			Files.Add(State.LocalFilename);
		}
	}
	if (Files.Num() > 0)
	{
		bResult = RunCommand(TEXT("history"), Parameters, Files, Results, Errors);
		if (bResult)
		{
			FXmlFile XmlFile;
			{
				TRACE_CPUPROFILER_EVENT_SCOPE(UnityVersionControlUtils::RunGetHistory::FXmlFile::LoadFile);
				bResult = XmlFile.LoadFile(Results, EConstructMethod::ConstructFromBuffer);
			}
			if (bResult)
			{
				bResult = ParseHistoryResults(bInUpdateHistory, XmlFile, InOutStates);
			}
			else
			{
				UE_LOG(LogSourceControl, Error, TEXT("RunGetHistory: XML parse error '%s'"), *XmlFile.GetLastError())
			}
		}
		if (!Errors.IsEmpty())
		{
			OutErrorMessages.Add(MoveTemp(Errors));
		}
	}

	return bResult;
}


/* Parse results of the 'cm update --xml=tempfile.xml --encoding="utf-8"' command.
 *
 * Results of the update command looks like that:
<UpdatedItems>
  <List>
	<UpdatedItem>
	  <Path>c:\Workspace\UE5PlasticPluginDev\Content\NewFolder\BP_CheckedOut.uasset</Path>
	  <User>sebastien.rombauts@unity3d.com</User>
	  <Changeset>94</Changeset>
	  <Date>2022-10-27T11:58:02+02:00</Date>
	</UpdatedItem>
  </List>
</UpdatedItems>
*/
static bool ParseUpdateResults(const FXmlFile& InXmlResult, TArray<FString>& OutFiles)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(UnityVersionControlUtils::ParseUpdateResults);

	static const FString UpdatedItems(TEXT("UpdatedItems"));
	static const FString List(TEXT("List"));
	static const FString UpdatedItem(TEXT("UpdatedItem"));
	static const FString Path(TEXT("Path"));

	const FXmlNode* UpdatedItemsNode = InXmlResult.GetRootNode();
	if (UpdatedItemsNode == nullptr || UpdatedItemsNode->GetTag() != UpdatedItems)
	{
		return false;
	}

	const FXmlNode* ListNode = UpdatedItemsNode->FindChildNode(List);
	if (ListNode == nullptr)
	{
		return false;
	}

	const TArray<FXmlNode*>& UpdatedItemNodes = ListNode->GetChildrenNodes();
	for (const FXmlNode* UpdatedItemNode : UpdatedItemNodes)
	{
		if (const FXmlNode* PathNode = UpdatedItemNode->FindChildNode(Path))
		{
			FString Filename = PathNode->GetContent();
			FPaths::NormalizeFilename(Filename);
			OutFiles.Add(Filename);
		}
	}

	return true;
}


/* Parse results of the 'cm partial update --report --machinereadable' command.
 *
 * Results of the update command looks like that:
STAGE Plastic is updating your workspace. Wait a moment, please...
STAGE Updated 63.01 KB of 63.01 KB (12 of 12 files to download / 16 of 21 operations to apply) /Content/Collections/SebSharedCollection.collection
AD c:\Workspace\UE5PlasticPluginDev\Content\LevelPrototyping\Materials\MI_Solid_Red.uasset
CH c:\Workspace\UE5PlasticPluginDev\Config\DefaultEditor.ini
DE c:\Workspace\UE5PlasticPluginDev\Content\Collections\SebSharedCollection.collection
*/
static bool ParseUpdateResults(const TArray<FString>& InResults, TArray<FString>& OutFiles)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(UnityVersionControlUtils::ParseUpdateResultsString);

	static const FString Stage = TEXT("STAGE ");
	static const int32 PrefixLen = 3; // "XX " typically "CH ", "AD " or "DE "

	for (const FString& Result : InResults)
	{
		if (Result.StartsWith(Stage))
			continue;

		FString Filename = Result.RightChop(PrefixLen);
		FPaths::NormalizeFilename(Filename);
		OutFiles.Add(Filename);
	}

	return true;
}

// Run a Plastic "update" command to sync the workspace and parse its XML results.
bool RunUpdate(const TArray<FString>& InFiles, const bool bInIsPartialWorkspace, TArray<FString>& OutUpdatedFiles, TArray<FString>& OutErrorMessages)
{
	bool bResult = false;

	TArray<FString> Parameters;
	// Update specified directory to the head of the repository
	// Detect special case for a partial checkout (CS:-1 in Gluon mode)!
	if (!bInIsPartialWorkspace)
	{
		const FScopedTempFile TempFile;
		TArray<FString> InfoMessages;
		Parameters.Add(FString::Printf(TEXT("--xml=\"%s\""), *TempFile.GetFilename()));
		Parameters.Add(TEXT("--encoding=\"utf-8\""));
		Parameters.Add(TEXT("--last"));
		Parameters.Add(TEXT("--dontmerge"));
		bResult = UnityVersionControlUtils::RunCommand(TEXT("update"), Parameters, TArray<FString>(), InfoMessages, OutErrorMessages);
		if (bResult)
		{
			// Load and parse the result of the update command
			FString Results;
			if (FFileHelper::LoadFileToString(Results, *TempFile.GetFilename()))
			{
				FXmlFile XmlFile;
				{
					TRACE_CPUPROFILER_EVENT_SCOPE(UnityVersionControlUtils::RunUpdate::FXmlFile::LoadFile);
					bResult = XmlFile.LoadFile(Results, EConstructMethod::ConstructFromBuffer);
				}
				if (bResult)
				{
					bResult = ParseUpdateResults(XmlFile, OutUpdatedFiles);
				}
				else
				{
					UE_LOG(LogSourceControl, Error, TEXT("RunUpdate: XML parse error '%s'"), *XmlFile.GetLastError())
				}
			}
		}
	}
	else
	{
		TArray<FString> Results;
		Parameters.Add(TEXT("--report"));
		Parameters.Add(TEXT("--machinereadable"));
		bResult = UnityVersionControlUtils::RunCommand(TEXT("partial update"), Parameters, InFiles, Results, OutErrorMessages);
		if (bResult)
		{
			bResult = ParseUpdateResults(Results, OutUpdatedFiles);
		}
	}

	return bResult;
}

#if ENGINE_MAJOR_VERSION == 5

/**
 * Parse results of the 'cm status --changelists --controlledchanged --noheader --xml --encoding="utf-8"' command.
 *
 * Results of the status changelists command looks like that:
<StatusOutput>
  <WkConfigType>Branch</WkConfigType>
  <WkConfigName>/main@rep:UEPlasticPluginDev@repserver:test@cloud</WkConfigName>
  <Changelists>
	<Changelist>
	  <Name>Default</Name>
	  <Description>Default Unity Version Control changelist</Description>
	  <Changes>
		<Change>
		  <Type>CO</Type>
		  <TypeVerbose>Checked-out</TypeVerbose>
		  <Path>UEPlasticPluginDev.uproject</Path>
		  <OldPath />
		  <PrintableMovedPath />
		  <MergesInfo />
		  <SimilarityPerUnit>0</SimilarityPerUnit>
		  <Similarity />
		  <Size>583</Size>
		  <PrintableSize>583 bytes</PrintableSize>
		  <PrintableLastModified>6 days ago</PrintableLastModified>
		  <RevisionType>enTextFile</RevisionType>
		  <LastModified>2022-06-07T12:28:32+02:00</LastModified>
		</Change>
		[...]
		<Change>
		</Change>
	  </Changes>
	</Changelist>
  </Changelists>
</StatusOutput>
*/
static bool ParseChangelistsResults(const FXmlFile& InXmlResult, TArray<FUnityVersionControlChangelistState>& OutChangelistsStates, TArray<TArray<FUnityVersionControlState>>& OutCLFilesStates)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(UnityVersionControlUtils::ParseChangelistsResults);

	static const FString StatusOutput(TEXT("StatusOutput"));
	static const FString WkConfigType(TEXT("WkConfigType"));
	static const FString WkConfigName(TEXT("WkConfigName"));
	static const FString Changelists(TEXT("Changelists"));
	static const FString Changelist(TEXT("Changelist"));
	static const FString Name(TEXT("Name"));
	static const FString Description(TEXT("Description"));
	static const FString Changes(TEXT("Changes"));
	static const FString Change(TEXT("Change"));
	static const FString Type(TEXT("Type"));
	static const FString Path(TEXT("Path"));

	const FString& WorkspaceRoot = FUnityVersionControlModule::Get().GetProvider().GetPathToWorkspaceRoot();

	const FXmlNode* StatusOutputNode = InXmlResult.GetRootNode();
	if (StatusOutputNode == nullptr || StatusOutputNode->GetTag() != StatusOutput)
	{
		return false;
	}

	const FXmlNode* ChangelistsNode = StatusOutputNode->FindChildNode(Changelists);
	if (ChangelistsNode)
	{
		const TArray<FXmlNode*>& ChangelistNodes = ChangelistsNode->GetChildrenNodes();
		OutCLFilesStates.SetNum(ChangelistNodes.Num());
		for (int32 ChangelistIndex = 0; ChangelistIndex < ChangelistNodes.Num(); ChangelistIndex++)
		{
			const FXmlNode* ChangelistNode = ChangelistNodes[ChangelistIndex];
			check(ChangelistNode);
			const FXmlNode* NameNode = ChangelistNode->FindChildNode(Name);
			const FXmlNode* DescriptionNode = ChangelistNode->FindChildNode(Description);
			const FXmlNode* ChangesNode = ChangelistNode->FindChildNode(Changes);
			if (NameNode == nullptr || DescriptionNode == nullptr || ChangesNode == nullptr)
			{
				continue;
			}

			FString NameTemp = UnityVersionControlUtils::DecodeXmlEntities(NameNode->GetContent());
			FUnityVersionControlChangelist ChangelistTemp(MoveTemp(NameTemp), true);
			FString DescriptionTemp = ChangelistTemp.IsDefault() ? FString() : UnityVersionControlUtils::DecodeXmlEntities(DescriptionNode->GetContent());
			FUnityVersionControlChangelistState ChangelistState(MoveTemp(ChangelistTemp), MoveTemp(DescriptionTemp));

			const TArray<FXmlNode*>& ChangeNodes = ChangesNode->GetChildrenNodes();
			for (const FXmlNode* ChangeNode : ChangeNodes)
			{
				check(ChangeNode);
				const FXmlNode* PathNode = ChangeNode->FindChildNode(Path);
				if (PathNode == nullptr)
				{
					continue;
				}

				// Here we make sure to only collect file states, not directories, since we shouldn't display the added directories to the Editor
				FString FileName = PathNode->GetContent();
				int32 DotIndex;
				if (FileName.FindChar(TEXT('.'), DotIndex))
				{
					FUnityVersionControlState FileState(FPaths::ConvertRelativePathToFull(WorkspaceRoot, MoveTemp(FileName)));
					FileState.Changelist = ChangelistState.Changelist;
					OutCLFilesStates[ChangelistIndex].Add(MoveTemp(FileState));
				}
			}

			OutChangelistsStates.Add(ChangelistState);
		}
	}

	if (!OutChangelistsStates.FindByPredicate(
		[](const FUnityVersionControlChangelistState& CLState) { return CLState.Changelist.IsDefault(); }
	))
	{
		// No Default Changelists isn't an error, but the Editor UX expects to always the Default changelist (so you can always move files back to it)
		FUnityVersionControlChangelistState DefaultChangelistState(FUnityVersionControlChangelist::DefaultChangelist);
		OutChangelistsStates.Insert(DefaultChangelistState, 0);
		OutCLFilesStates.Insert(TArray<FUnityVersionControlState>(), 0);
	}

	return true;
}

// Run a Plastic "status --changelist --xml" and parse its XML result.
bool RunGetChangelists(TArray<FUnityVersionControlChangelistState>& OutChangelistsStates, TArray<TArray<FUnityVersionControlState>>& OutCLFilesStates, TArray<FString>& OutErrorMessages)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(UnityVersionControlUtils::RunGetChangelists);

	FString Results;
	FString Errors;
	TArray<FString> Parameters;
	Parameters.Add(TEXT("--changelists"));
	Parameters.Add(TEXT("--controlledchanged"));
	Parameters.Add(TEXT("--noheader"));
	Parameters.Add(TEXT("--xml"));
	Parameters.Add(TEXT("--encoding=\"utf-8\""));
	bool bResult = RunCommand(TEXT("status"), Parameters, TArray<FString>(), Results, Errors);
	if (bResult)
	{
		FXmlFile XmlFile;
		{
			TRACE_CPUPROFILER_EVENT_SCOPE(UnityVersionControlUtils::RunGetChangelists::FXmlFile::LoadFile);
			bResult = XmlFile.LoadFile(Results, EConstructMethod::ConstructFromBuffer);
		}
		if (bResult)
		{
			bResult = ParseChangelistsResults(XmlFile, OutChangelistsStates, OutCLFilesStates);
		}
		else
		{
			UE_LOG(LogSourceControl, Error, TEXT("RunGetChangelists: XML parse error '%s'"), *XmlFile.GetLastError())
		}
	}
	if (!Errors.IsEmpty())
	{
		OutErrorMessages.Add(MoveTemp(Errors));
	}

	return bResult;
}

// Parse the one letter file status in front of each line of the 'cm diff sh:<ShelveId>'
EWorkspaceState ParseShelveFileStatus(const TCHAR InFileStatus)
{
	if (InFileStatus == 'A') // Added
	{
		return EWorkspaceState::Added;
	}
	else if (InFileStatus == 'D') // Deleted
	{
		return EWorkspaceState::Deleted;
	}
	else if (InFileStatus == 'C') // Changed (CheckedOut or not)
	{
		return EWorkspaceState::CheckedOutChanged;
	}
	else if (InFileStatus == 'M') // Moved/Renamed (or Locally Moved)
	{
		return EWorkspaceState::Moved;
	}
	else
	{
		UE_LOG(LogSourceControl, Warning, TEXT("Unknown file status '%c'"), InFileStatus);
		return EWorkspaceState::Unknown;
	}
}

void AddShelvedFileToChangelist(FUnityVersionControlChangelistState& InOutChangelistsState, FString&& InFilename, EWorkspaceState InShelveStatus, FString&& InMovedFrom)
{
	TSharedRef<FUnityVersionControlState, ESPMode::ThreadSafe> ShelveState = MakeShared<FUnityVersionControlState>(MoveTemp(InFilename), InShelveStatus);
	ShelveState->MovedFrom = MoveTemp(InMovedFrom);

	// Add one revision to be able to fetch the shelved file for diff, if it's not marked for deletion.
	if (InShelveStatus != EWorkspaceState::Deleted)
	{
		const TSharedRef<FUnityVersionControlRevision, ESPMode::ThreadSafe> SourceControlRevision = MakeShared<FUnityVersionControlRevision>();
		SourceControlRevision->State = &ShelveState.Get();
		SourceControlRevision->Filename = ShelveState->GetFilename();
		SourceControlRevision->ShelveId = InOutChangelistsState.ShelveId;
		SourceControlRevision->ChangesetNumber = InOutChangelistsState.ShelveId; // Note: for display in the diff window only
		SourceControlRevision->Date = InOutChangelistsState.ShelveDate; // Note: not yet used for display as of UE5.2

		ShelveState->History.Add(SourceControlRevision);
	}

	// In case of a Moved file, it would appear twice in the list, so overwrite it if already in
	if (FSourceControlStateRef* ExistingShelveState = InOutChangelistsState.ShelvedFiles.FindByPredicate(
		[&ShelveState](const FSourceControlStateRef& State)
		{
			return State->GetFilename().Equals(ShelveState->GetFilename());
		}))
	{
		*ExistingShelveState = MoveTemp(ShelveState);
	}
	else
	{
		InOutChangelistsState.ShelvedFiles.Add(MoveTemp(ShelveState));
	}
}


/**
 * Parse results of the 'cm diff sh:<ShelveId>' command.
 *
 * Results of the diff command looks like that:
C "Content\NewFolder\BP_CheckedOut.uasset"
C "Content\NewFolder\BP_Renamed.uasset"
A "Content\NewFolder\BP_ControlledUnchanged.uasset"
D "Content\NewFolder\BP_Changed.uasset"
M "Content\NewFolder\BP_ControlledUnchanged.uasset" "Content\NewFolder\BP_Renamed.uasset"
*/
bool ParseShelveDiffResults(const FString InWorkspaceRoot, TArray<FString>&& InResults, FUnityVersionControlChangelistState& InOutChangelistsState)
{
	bool bCommandSuccessful = true;

	InOutChangelistsState.ShelvedFiles.Reset(InResults.Num());
	for (FString& Result : InResults)
	{
		EWorkspaceState ShelveState = ParseShelveFileStatus(Result[0]);

		// Remove outer double quotes
		Result.MidInline(3, Result.Len() - 4, false);

		FString MovedFrom;
		if (ShelveState == EWorkspaceState::Moved)
		{
			// Search for the inner double quotes in the middle of "Content/Source.uasset" "Content/Destination.uasset" to keep only the destination filename
			int32 RenameIndex;
			if (Result.FindLastChar(TEXT('"'), RenameIndex))
			{
				MovedFrom = Result.Left(RenameIndex - 2);
				MovedFrom = FPaths::ConvertRelativePathToFull(InWorkspaceRoot, MovedFrom);
				Result.RightChopInline(RenameIndex + 1);
			}
		}

		if (ShelveState != EWorkspaceState::Unknown && !Result.IsEmpty())
		{
			FString AbsoluteFilename = FPaths::ConvertRelativePathToFull(InWorkspaceRoot, MoveTemp(Result));
			AddShelvedFileToChangelist(InOutChangelistsState, MoveTemp(AbsoluteFilename), ShelveState, MoveTemp(MovedFrom));
		}
		else
		{
			bCommandSuccessful = false;
		}
	}

	return bCommandSuccessful;
}

/**
 * Run for each shelve a "diff sh:<ShelveId>" and parse their result to list their files.
 * @param	InOutChangelistsStates	The list of changelists, filled with their shelved files
 * @param	OutErrorMessages		Any errors (from StdErr) as an array per-line
 */
bool RunGetShelveFiles(TArray<FUnityVersionControlChangelistState>& InOutChangelistsStates, TArray<FString>& OutErrorMessages)
{
	bool bCommandSuccessful = true;

	const FString& WorkspaceRoot = FUnityVersionControlModule::Get().GetProvider().GetPathToWorkspaceRoot();

	for (FUnityVersionControlChangelistState& ChangelistState : InOutChangelistsStates)
	{
		if (ChangelistState.ShelveId != ISourceControlState::INVALID_REVISION)
		{
			TArray<FString> Results;
			TArray<FString> Parameters;
			// TODO switch to custom format --format="{status};{path};{srccmpath}" for better parsing, and perhaps reusing code
			Parameters.Add(FString::Printf(TEXT("sh:%d"), ChangelistState.ShelveId));
			const bool bDiffSuccessful = UnityVersionControlUtils::RunCommand(TEXT("diff"), Parameters, TArray<FString>(), Results, OutErrorMessages);
			if (bDiffSuccessful)
			{
				bCommandSuccessful = ParseShelveDiffResults(WorkspaceRoot, MoveTemp(Results), ChangelistState);
			}
		}
	}

	return bCommandSuccessful;
}

/**
 * Parse results of the 'cm find "shelves where owner='me'" --xml --encoding="utf-8"' command.
 *
 * Find shelves with comments starting like "ChangelistXXX: " and matching an existing Changelist number XXX
 *
 * Results of the find command looks like the following; note the "Changelist67: " prefix of the comment:
<?xml version="1.0" encoding="utf-8" ?>
<PLASTICQUERY>
  <SHELVE>
	<ID>1376</ID>
	<SHELVEID>9</SHELVEID>
    <COMMENT>Changelist67: test by Sebastien</COMMENT>
	<DATE>2022-06-30T16:39:55+02:00</DATE>
    <OWNER>sebastien.rombauts@unity3d.com</OWNER>
    <REPOSITORY>UE5PlasticPluginDev</REPOSITORY>
    <REPNAME>UE5PlasticPluginDev</REPNAME>
    <REPSERVER>test@cloud</REPSERVER>
	<PARENT>45</PARENT>
	<GUID>8fbefbcc-81a7-4b81-9b99-b51f4873d09f</GUID>
  </SHELVE>
  [...]
</PLASTICQUERY>
*/
static bool ParseShelvesResults(const FXmlFile& InXmlResult, TArray<FUnityVersionControlChangelistState>& InOutChangelistsStates)
{
	static const FString PlasticQuery(TEXT("PLASTICQUERY"));
	static const FString Shelve(TEXT("SHELVE"));
	static const FString ShelveId(TEXT("SHELVEID"));
	static const FString Date(TEXT("DATE"));
	static const FString Comment(TEXT("COMMENT"));

	const FXmlNode* PlasticQueryNode = InXmlResult.GetRootNode();
	if (PlasticQueryNode == nullptr || PlasticQueryNode->GetTag() != PlasticQuery)
	{
		return false;
	}

	const TArray<FXmlNode*>& ShelvesNodes = PlasticQueryNode->GetChildrenNodes();
	for (const FXmlNode* ShelveNode : ShelvesNodes)
	{
		check(ShelveNode);
		const FXmlNode* ShelveIdNode = ShelveNode->FindChildNode(ShelveId);
		const FXmlNode* CommentNode = ShelveNode->FindChildNode(Comment);
		if (ShelveIdNode == nullptr || CommentNode == nullptr)
		{
			continue;
		}

		const FString& ShelveIdString = ShelveIdNode->GetContent();
		const FString& CommentString = CommentNode->GetContent();

		// Search if there is a changelist matching the shelve (that is, a shelve with a comment starting with "ChangelistXXX: ")
		for (FUnityVersionControlChangelistState& ChangelistState : InOutChangelistsStates)
		{
			FUnityVersionControlChangelistRef Changelist = StaticCastSharedRef<FUnityVersionControlChangelist>(ChangelistState.GetChangelist());
			const FString ChangelistPrefix = FString::Printf(TEXT("Changelist%s: "), *Changelist->GetName());
			if (CommentString.StartsWith(ChangelistPrefix))
			{
				ChangelistState.ShelveId = FCString::Atoi(*ShelveIdString);

				if (const FXmlNode* DateNode = ShelveNode->FindChildNode(Date))
				{
					const FString& DateIso = DateNode->GetContent();
					FDateTime::ParseIso8601(*DateIso, ChangelistState.ShelveDate);
				}
			}
		}
	}

	return true;
}

// Run find "shelves where owner='me'" and for each shelve matching a changelist a "diff sh:<ShelveId>" and parse their results.
bool RunGetShelves(TArray<FUnityVersionControlChangelistState>& InOutChangelistsStates, TArray<FString>& OutErrorMessages)
{
	bool bCommandSuccessful;

	FString Results;
	FString Errors;
	TArray<FString> Parameters;
	Parameters.Add(TEXT("\"shelves where owner = 'me'\""));
	Parameters.Add(TEXT("--xml"));
	Parameters.Add(TEXT("--encoding=\"utf-8\""));
	bCommandSuccessful = UnityVersionControlUtils::RunCommand(TEXT("find"), Parameters, TArray<FString>(), Results, Errors);
	if (bCommandSuccessful)
	{
		FXmlFile XmlFile;
		bCommandSuccessful = XmlFile.LoadFile(Results, EConstructMethod::ConstructFromBuffer);
		if (bCommandSuccessful)
		{
			bCommandSuccessful = ParseShelvesResults(XmlFile, InOutChangelistsStates);
			if (bCommandSuccessful)
			{
				bCommandSuccessful = RunGetShelveFiles(InOutChangelistsStates, OutErrorMessages);
			}
		}
	}
	if (!Errors.IsEmpty())
	{
		OutErrorMessages.Add(MoveTemp(Errors));
	}

	return bCommandSuccessful;
}

/**
 * Parse results of the 'cm diff sh:<ShelveId> --format="{status};{baserevid};{path}"' command.
 *
 * Results of the diff command looks like that:
C;666;Content\NewFolder\BP_CheckedOut.uasset
 * but for Moved assets there are two entires that we need to merge:
C;266;"Content\ThirdPerson\Blueprints\BP_ThirdPersonCharacterRenamed.uasset"
M;-1;"Content\ThirdPerson\Blueprints\BP_ThirdPersonCharacterRenamed.uasset"
*/
bool ParseShelveDiffResults(const FString InWorkspaceRoot, TArray<FString>&& InResults, TArray<FUnityVersionControlRevision>& OutBaseRevisions)
{
	bool bCommandSuccessful = true;

	OutBaseRevisions.Reset(InResults.Num());
	for (FString& InResult : InResults)
	{
		TArray<FString> ResultElements;
		ResultElements.Reserve(3);
		InResult.ParseIntoArray(ResultElements, FILE_STATUS_SEPARATOR, false); // Don't cull empty values in csv
		if (ResultElements.Num() == 3 && ResultElements[0].Len() == 1)
		{
			EWorkspaceState ShelveState = ParseShelveFileStatus(ResultElements[0][0]);
			const int32 BaseRevisionId = FCString::Atoi(*ResultElements[1]);
			// Remove outer double quotes on filename
			FString File = MoveTemp(ResultElements[2]);
			File.MidInline(1, File.Len() - 2, false);
			FString AbsoluteFilename = FPaths::ConvertRelativePathToFull(InWorkspaceRoot, File);

			if (ShelveState == EWorkspaceState::Moved)
			{
				// In case of a Moved file, it appears twice in the list, so update the first entry (set as a "Changed" but has the Base Revision Id) and update it with the "Move" status
				if (FUnityVersionControlRevision* ExistingShelveRevision = OutBaseRevisions.FindByPredicate(
					[&AbsoluteFilename](const FUnityVersionControlRevision& State)
					{
						return State.GetFilename().Equals(AbsoluteFilename);
					}))
				{
					ExistingShelveRevision->Action = FileStateToAction(EWorkspaceState::Moved);
					continue;
				}
			}

			FUnityVersionControlRevision SourceControlRevision;
			SourceControlRevision.Filename = MoveTemp(AbsoluteFilename);
			SourceControlRevision.Action = FileStateToAction(ShelveState);
			SourceControlRevision.RevisionId = BaseRevisionId;
			OutBaseRevisions.Add(MoveTemp(SourceControlRevision));
		}
		else
		{
			bCommandSuccessful = false;
		}
	}

	return bCommandSuccessful;
}

/**
 * Run for a shelve a "diff sh:<ShelveId> --format='{status};{baserevid};{path}'" and parse the result to list its files.
 * @param	InOutChangelistsStates	The list of changelists, filled with their shelved files
 * @param	OutErrorMessages		Any errors (from StdErr) as an array per-line
 */
bool RunGetShelveFiles(const int32 InShelveId, TArray<FUnityVersionControlRevision>& OutBaseRevisions, TArray<FString>& OutErrorMessages)
{
	bool bCommandSuccessful = true;

	const FString& WorkspaceRoot = FUnityVersionControlModule::Get().GetProvider().GetPathToWorkspaceRoot();

	if (InShelveId != ISourceControlState::INVALID_REVISION)
	{
		TArray<FString> Results;
		TArray<FString> Parameters;
		Parameters.Add(FString::Printf(TEXT("sh:%d"), InShelveId));
		Parameters.Add(TEXT("--format=\"{status};{baserevid};{path}\""));
		Parameters.Add(TEXT("--encoding=\"utf-8\""));
		const bool bDiffSuccessful = UnityVersionControlUtils::RunCommand(TEXT("diff"), Parameters, TArray<FString>(), Results, OutErrorMessages);
		if (bDiffSuccessful)
		{
			bCommandSuccessful = ParseShelveDiffResults(WorkspaceRoot, MoveTemp(Results), OutBaseRevisions);
		}
	}

	return bCommandSuccessful;
}

/**
 * Parse results of the 'cm find "shelves where ShelveId='NNN'" --xml --encoding="utf-8"' command.
 *
 * Results of the find command looks like the following; note the "Changelist67: " prefix of the comment:
<?xml version="1.0" encoding="utf-8" ?>
<PLASTICQUERY>
  <SHELVE>
	<ID>1376</ID>
	<SHELVEID>9</SHELVEID>
	<COMMENT>Changelist67: test by Sebastien</COMMENT>
	<DATE>2022-06-30T16:39:55+02:00</DATE>
	<OWNER>sebastien.rombauts@unity3d.com</OWNER>
	<REPOSITORY>UE5PlasticPluginDev</REPOSITORY>
	<REPNAME>UE5PlasticPluginDev</REPNAME>
	<REPSERVER>test@cloud</REPSERVER>
	<PARENT>45</PARENT>
	<GUID>8fbefbcc-81a7-4b81-9b99-b51f4873d09f</GUID>
  </SHELVE>
  [...]
</PLASTICQUERY>
*/
static bool ParseShelvesResult(const FXmlFile& InXmlResult, int32& OutShelveId, FString& OutComment, FDateTime& OutDate, FString& OutOwner)
{
	static const FString PlasticQuery(TEXT("PLASTICQUERY"));
	static const FString Shelve(TEXT("SHELVE"));
	static const FString ShelveId(TEXT("SHELVEID"));
	static const FString Comment(TEXT("COMMENT"));
	static const FString Date(TEXT("DATE"));

	const FXmlNode* PlasticQueryNode = InXmlResult.GetRootNode();
	if (PlasticQueryNode == nullptr || PlasticQueryNode->GetTag() != PlasticQuery)
	{
		return false;
	}

	const TArray<FXmlNode*>& ShelvesNodes = PlasticQueryNode->GetChildrenNodes();
	if (ShelvesNodes.Num() < 1)
	{
		return false;
	}

	if (const FXmlNode* ShelveNode = ShelvesNodes[0])
	{
		check(ShelveNode);
		if (const FXmlNode* ShelveIdNode = ShelveNode->FindChildNode(ShelveId))
		{
			OutShelveId = FCString::Atoi(*ShelveIdNode->GetContent());
		}
		if (const FXmlNode* CommentNode = ShelveNode->FindChildNode(Comment))
		{
			OutComment = DecodeXmlEntities(CommentNode->GetContent());
		}
		if (const FXmlNode* DateNode = ShelveNode->FindChildNode(Date))
		{
			FDateTime::ParseIso8601(*DateNode->GetContent(), OutDate);
		}
	}

	return true;
}

bool RunGetShelve(const int32 InShelveId, FString& OutComment, FDateTime& OutDate, FString& OutOwner, TArray<FUnityVersionControlRevision>& OutBaseRevisions, TArray<FString>& OutErrorMessages)
{
	bool bCommandSuccessful;

	FString Results;
	FString Errors;
	TArray<FString> Parameters;
	Parameters.Add(FString::Printf(TEXT("\"shelves where ShelveId = %d\""), InShelveId));
	Parameters.Add(TEXT("--xml"));
	Parameters.Add(TEXT("--encoding=\"utf-8\""));
	bCommandSuccessful = UnityVersionControlUtils::RunCommand(TEXT("find"), Parameters, TArray<FString>(), Results, Errors);
	if (bCommandSuccessful)
	{
		FXmlFile XmlFile;
		bCommandSuccessful = XmlFile.LoadFile(Results, EConstructMethod::ConstructFromBuffer);
		if (bCommandSuccessful)
		{
			int32 ShelveId;
			bCommandSuccessful = ParseShelvesResult(XmlFile, ShelveId, OutComment, OutDate, OutOwner);
			if (bCommandSuccessful)
			{
				bCommandSuccessful = RunGetShelveFiles(InShelveId, OutBaseRevisions, OutErrorMessages);
			}
		}
	}
	if (!Errors.IsEmpty())
	{
		OutErrorMessages.Add(MoveTemp(Errors));
	}

	return bCommandSuccessful;
}

#endif

bool UpdateCachedStates(TArray<FUnityVersionControlState>&& InStates)
{
	FUnityVersionControlProvider& Provider = FUnityVersionControlModule::Get().GetProvider();
	const FDateTime Now = FDateTime::Now();

	for (auto&& InState : InStates)
	{
		TSharedRef<FUnityVersionControlState, ESPMode::ThreadSafe> State = Provider.GetStateInternal(InState.LocalFilename);
		*State = MoveTemp(InState);
		State->TimeStamp = Now;
	}

	return (InStates.Num() > 0);
}

/**
 * Helper struct for RemoveRedundantErrors()
 */
struct FRemoveRedundantErrors
{
	explicit FRemoveRedundantErrors(const FString& InFilter)
		: Filter(InFilter)
	{
	}

	bool operator()(const FString& String) const
	{
		if (String.Contains(Filter))
		{
			return true;
		}

		return false;
	}

	/** The filter string we try to identify in the reported error */
	FString Filter;
};

void RemoveRedundantErrors(FUnityVersionControlCommand& InCommand, const FString& InFilter)
{
	bool bFoundRedundantError = false;
	for (const FString& ErrorMessage : InCommand.ErrorMessages)
	{
		if (ErrorMessage.Contains(InFilter, ESearchCase::CaseSensitive, ESearchDir::FromEnd))
		{
			InCommand.InfoMessages.Add(ErrorMessage);
			bFoundRedundantError = true;
		}
	}

	InCommand.ErrorMessages.RemoveAll(FRemoveRedundantErrors(InFilter));

	// if we have no error messages now, assume success!
	if (bFoundRedundantError && InCommand.ErrorMessages.Num() == 0 && !InCommand.bCommandSuccessful)
	{
		InCommand.bCommandSuccessful = true;
	}
}

void SwitchVerboseLogs(const bool bInEnable)
{
	if (bInEnable && LogSourceControl.GetVerbosity() < ELogVerbosity::Verbose)
	{
		LogSourceControl.SetVerbosity(ELogVerbosity::Verbose);
	}
	else if (!bInEnable && LogSourceControl.GetVerbosity() == ELogVerbosity::Verbose)
	{
		LogSourceControl.SetVerbosity(ELogVerbosity::Log);
	}
}

} // namespace UnityVersionControlUtils
