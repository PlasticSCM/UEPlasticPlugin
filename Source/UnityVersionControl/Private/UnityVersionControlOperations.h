// Copyright (c) 2023 Unity Technologies

// Specialization of classes defined in Engine\Source\Developer\SourceControl\Public\SourceControlOperations.h

#pragma once

#include "CoreMinimal.h"
#include "IUnityVersionControlWorker.h"
#include "UnityVersionControlRevision.h"
#include "UnityVersionControlState.h"
#include "SourceControlOperationBase.h"
#include "SourceControlOperations.h"

#include "Runtime/Launch/Resources/Version.h"

#if ENGINE_MAJOR_VERSION == 5
#include "UnityVersionControlChangelist.h"
#include "UnityVersionControlChangelistState.h"
#endif

class FUnityVersionControlProvider;
typedef TSharedRef<class FUnityVersionControlBranch, ESPMode::ThreadSafe> FUnityVersionControlBranchRef;
typedef TSharedRef<class FUnityVersionControlLock, ESPMode::ThreadSafe> FUnityVersionControlLockRef;


/**
 * Internal operation used to revert checked-out unchanged files
*/
// NOTE: added to Engine in Unreal Engine 5 for changelists
class FPlasticRevertUnchanged final : public FSourceControlOperationBase
{
public:
	// ISourceControlOperation interface
	virtual FName GetName() const override;

	virtual FText GetInProgressString() const override;
};


/**
 * Internal operation used to sync all files in the workspace
*/
class FPlasticSyncAll final : public FSync
{
public:
	// ISourceControlOperation interface
	virtual FName GetName() const override;

	/** List of files updated by the operation */
	TArray<FString> UpdatedFiles;
};


/**
 * Internal operation used to revert checked-out files
*/
class FPlasticRevertAll final : public FRevert
{
public:
	// ISourceControlOperation interface
	virtual FName GetName() const override;

	virtual FText GetInProgressString() const override;

	/** List of files updated by the operation */
	TArray<FString> UpdatedFiles;
};


/**
* Internal operation used to create a new Workspace and a new Repository
*/
class FPlasticMakeWorkspace final : public FSourceControlOperationBase
{
public:
	// ISourceControlOperation interface
	virtual FName GetName() const override;

	virtual FText GetInProgressString() const override;

	FString WorkspaceName;
	FString RepositoryName;
	FString ServerUrl;
	bool bPartialWorkspace = false;
};


/**
 * Internal operation used to switch to a partial workspace
*/
class FPlasticSwitchToPartialWorkspace final : public FSourceControlOperationBase
{
public:
	// ISourceControlOperation interface
	virtual FName GetName() const override;

	virtual FText GetInProgressString() const override;
};


/**
 * Internal operation to list locks, aka "cm lock list"
*/
class FPlasticGetLocks final : public FSourceControlOperationBase
{
public:
	// ISourceControlOperation interface
	virtual FName GetName() const override;

	virtual FText GetInProgressString() const override;

	// List of locks found
	TArray<FUnityVersionControlLockRef> Locks;
};


/**
 * Internal operation used to release or remove Lock(s) on file(s)
*/
class FPlasticUnlock final : public FSourceControlOperationBase
{
public:
	// ISourceControlOperation interface
	virtual FName GetName() const override;

	virtual FText GetInProgressString() const override;

	// Locks to unlock, including the Item Id and branch name
	TArray<FUnityVersionControlLockRef> Locks;

	// Release the Lock(s), and optionally remove (delete) them completely
	bool bRemove = false;
};


/**
 * Internal operation to list branches, aka "cm find branch"
*/
class FPlasticGetBranches final : public FSourceControlOperationBase
{
public:
	// ISourceControlOperation interface
	virtual FName GetName() const override;

	virtual FText GetInProgressString() const override;

	// Limit the list of branches to ones created from this date
	FDateTime FromDate;

	// List of branches found
	TArray<FUnityVersionControlBranchRef> Branches;
};


/**
 * Internal operation used to switch the workspace to another branch
*/
class FPlasticSwitchToBranch final : public FSourceControlOperationBase
{
public:
	// ISourceControlOperation interface
	virtual FName GetName() const override;

	virtual FText GetInProgressString() const override;

	// Branch to switch the workspace to
	FString BranchName;

	/** List of files updated by the operation */
	TArray<FString> UpdatedFiles;
};


/**
 * Internal operation used to merge a branch into the current branch
*/
class FPlasticMergeBranch final : public FSourceControlOperationBase
{
public:
	// ISourceControlOperation interface
	virtual FName GetName() const override;

	virtual FText GetInProgressString() const override;

	// Branch to switch the workspace to
	FString BranchName;

	/** List of files updated by the operation */
	TArray<FString> UpdatedFiles;
};


/**
 * Internal operation used to create a branch
*/
class FPlasticCreateBranch final : public FSourceControlOperationBase
{
public:
	// ISourceControlOperation interface
	virtual FName GetName() const override;

	virtual FText GetInProgressString() const override;

	FString BranchName;
	FString Comment;
};


/**
 * Internal operation used to rename a branch
*/
class FPlasticRenameBranch final : public FSourceControlOperationBase
{
public:
	// ISourceControlOperation interface
	virtual FName GetName() const override;

	virtual FText GetInProgressString() const override;

	FString OldName;
	FString NewName;
};


/**
 * Internal operation used to deletes branches
*/
class FPlasticDeleteBranches final : public FSourceControlOperationBase
{
public:
	// ISourceControlOperation interface
	virtual FName GetName() const override;

	virtual FText GetInProgressString() const override;

	TArray<FString> BranchNames;
};


/** Called when first activated on a project, and then at project load time.
 *  Look for the root directory of the Plastic workspace (where the ".plastic/" subdirectory is located). */
class FPlasticConnectWorker final : public IUnityVersionControlWorker
{
public:
	explicit FPlasticConnectWorker(FUnityVersionControlProvider& InSourceControlProvider)
		: IUnityVersionControlWorker(InSourceControlProvider)
	{}
	virtual ~FPlasticConnectWorker() = default;
	// IUnityVersionControlWorker interface
	virtual FName GetName() const override;
	virtual bool Execute(class FUnityVersionControlCommand& InCommand) override;
	virtual bool UpdateStates() override;

public:
	/** Temporary states for results */
	TArray<FUnityVersionControlState> States;
};

class FPlasticCheckOutWorker final : public IUnityVersionControlWorker
{
public:
	explicit FPlasticCheckOutWorker(FUnityVersionControlProvider& InSourceControlProvider)
		: IUnityVersionControlWorker(InSourceControlProvider)
#if ENGINE_MAJOR_VERSION == 5
		, InChangelist(FUnityVersionControlChangelist::DefaultChangelist) // By default, add checked out files in the default changelist.
#endif
	{}
	virtual ~FPlasticCheckOutWorker() = default;
	// IUnityVersionControlWorker interface
	virtual FName GetName() const override;
	virtual bool Execute(class FUnityVersionControlCommand& InCommand) override;
	virtual bool UpdateStates() override;

public:
	/** Temporary states for results */
	TArray<FUnityVersionControlState> States;

#if ENGINE_MAJOR_VERSION == 5
	/** Changelist we checked-out files to (defaults to the Default changelist) */
	FUnityVersionControlChangelist InChangelist;
#endif
};

/** Check-in a set of file to the local depot. */
class FPlasticCheckInWorker final : public IUnityVersionControlWorker
{
public:
	explicit FPlasticCheckInWorker(FUnityVersionControlProvider& InSourceControlProvider)
		: IUnityVersionControlWorker(InSourceControlProvider)
	{}
	virtual ~FPlasticCheckInWorker() = default;
	// IUnityVersionControlWorker interface
	virtual FName GetName() const override;
	virtual bool Execute(class FUnityVersionControlCommand& InCommand) override;
	virtual bool UpdateStates() override;

public:
	/** Temporary states for results */
	TArray<FUnityVersionControlState> States;

#if ENGINE_MAJOR_VERSION == 5
	/** Changelist we submitted */
	FUnityVersionControlChangelist InChangelist;
#endif
};

/** Add an untracked file to source control (so only a subset of the Plastic add command). */
class FPlasticMarkForAddWorker final : public IUnityVersionControlWorker
{
public:
	explicit FPlasticMarkForAddWorker(FUnityVersionControlProvider& InSourceControlProvider)
		: IUnityVersionControlWorker(InSourceControlProvider)
#if ENGINE_MAJOR_VERSION == 5
		, InChangelist(FUnityVersionControlChangelist::DefaultChangelist) // By default, add new files in the default changelist.
#endif
	{}
	virtual ~FPlasticMarkForAddWorker() = default;
	// IUnityVersionControlWorker interface
	virtual FName GetName() const override;
	virtual bool Execute(class FUnityVersionControlCommand& InCommand) override;
	virtual bool UpdateStates() override;

public:
	/** Temporary states for results */
	TArray<FUnityVersionControlState> States;

#if ENGINE_MAJOR_VERSION == 5
	/** Changelist we added files to (defaults to the Default changelist) */
	FUnityVersionControlChangelist InChangelist;
#endif
};

/** Delete a file and remove it from source control. */
class FPlasticDeleteWorker final : public IUnityVersionControlWorker
{
public:
	explicit FPlasticDeleteWorker(FUnityVersionControlProvider& InSourceControlProvider)
		: IUnityVersionControlWorker(InSourceControlProvider)
#if ENGINE_MAJOR_VERSION == 5
		, InChangelist(FUnityVersionControlChangelist::DefaultChangelist) // By default, add deleted files in the default changelist.
#endif
	{}
	virtual ~FPlasticDeleteWorker() = default;
	// IUnityVersionControlWorker interface
	virtual FName GetName() const override;
	virtual bool Execute(class FUnityVersionControlCommand& InCommand) override;
	virtual bool UpdateStates() override;

public:
	/** Temporary states for results */
	TArray<FUnityVersionControlState> States;

#if ENGINE_MAJOR_VERSION == 5
	/** Changelist we delete files to (defaults to the Default changelist) */
	FUnityVersionControlChangelist InChangelist;
#endif
};

/** Revert any change to a file to its state on the local depot. */
class FPlasticRevertWorker final : public IUnityVersionControlWorker
{
public:
	explicit FPlasticRevertWorker(FUnityVersionControlProvider& InSourceControlProvider)
		: IUnityVersionControlWorker(InSourceControlProvider)
	{}
	virtual ~FPlasticRevertWorker() = default;
	// IUnityVersionControlWorker interface
	virtual FName GetName() const override;
	virtual bool Execute(class FUnityVersionControlCommand& InCommand) override;
	virtual bool UpdateStates() override;

public:
	/** Temporary states for results */
	TArray<FUnityVersionControlState> States;
};

/** Revert only unchanged file(s) (uncheckout). */
class FPlasticRevertUnchangedWorker final : public IUnityVersionControlWorker
{
public:
	explicit FPlasticRevertUnchangedWorker(FUnityVersionControlProvider& InSourceControlProvider)
		: IUnityVersionControlWorker(InSourceControlProvider)
	{}
	virtual ~FPlasticRevertUnchangedWorker() = default;
	// IUnityVersionControlWorker interface
	virtual FName GetName() const override;
	virtual bool Execute(class FUnityVersionControlCommand& InCommand) override;
	virtual bool UpdateStates() override;

public:
	/** Temporary states for results */
	TArray<FUnityVersionControlState> States;
};

/** Revert all checked-out file(s). */
class FPlasticRevertAllWorker final : public IUnityVersionControlWorker
{
public:
	explicit FPlasticRevertAllWorker(FUnityVersionControlProvider& InSourceControlProvider)
		: IUnityVersionControlWorker(InSourceControlProvider)
	{}
	virtual ~FPlasticRevertAllWorker() = default;
	// IUnityVersionControlWorker interface
	virtual FName GetName() const override;
	virtual bool Execute(class FUnityVersionControlCommand& InCommand) override;
	virtual bool UpdateStates() override;

public:
	/** Temporary states for results */
	TArray<FUnityVersionControlState> States;
};

/** Create a new Workspace and a new Repository */
class FPlasticMakeWorkspaceWorker final : public IUnityVersionControlWorker
{
public:
	explicit FPlasticMakeWorkspaceWorker(FUnityVersionControlProvider& InSourceControlProvider)
		: IUnityVersionControlWorker(InSourceControlProvider)
	{}
	virtual ~FPlasticMakeWorkspaceWorker() = default;
	// IUnityVersionControlWorker interface
	virtual FName GetName() const override;
	virtual bool Execute(class FUnityVersionControlCommand& InCommand) override;
	virtual bool UpdateStates() override;
};

/** Switch to Partial Workspace. */
class FPlasticSwitchToPartialWorkspaceWorker final : public IUnityVersionControlWorker
{
public:
	explicit FPlasticSwitchToPartialWorkspaceWorker(FUnityVersionControlProvider& InSourceControlProvider)
		: IUnityVersionControlWorker(InSourceControlProvider)
	{}
	virtual ~FPlasticSwitchToPartialWorkspaceWorker() = default;
	// IUnityVersionControlWorker interface
	virtual FName GetName() const override;
	virtual bool Execute(class FUnityVersionControlCommand& InCommand) override;
	virtual bool UpdateStates() override;

public:
	/** Temporary states for results */
	TArray<FUnityVersionControlState> States;
};

/** list locks. */
class FPlasticGetLocksWorker final : public IUnityVersionControlWorker
{
public:
	explicit FPlasticGetLocksWorker(FUnityVersionControlProvider& InSourceControlProvider)
		: IUnityVersionControlWorker(InSourceControlProvider)
	{}
	virtual ~FPlasticGetLocksWorker() = default;
	// IUnityVersionControlWorker interface
	virtual FName GetName() const override;
	virtual bool Execute(class FUnityVersionControlCommand& InCommand) override;
	virtual bool UpdateStates() override;

	// Current branch the workspace is on (at the end of the operation)
	FString CurrentBranchName;
};

/** release or remove Lock(s) on file(s). */
class FPlasticUnlockWorker final : public IUnityVersionControlWorker
{
public:
	explicit FPlasticUnlockWorker(FUnityVersionControlProvider& InSourceControlProvider)
		: IUnityVersionControlWorker(InSourceControlProvider)
	{}
	virtual ~FPlasticUnlockWorker() = default;
	// IUnityVersionControlWorker interface
	virtual FName GetName() const override;
	virtual bool Execute(class FUnityVersionControlCommand& InCommand) override;
	virtual bool UpdateStates() override;

public:
	/** Temporary states for results */
	TArray<FUnityVersionControlState> States;
};

/** list branches. */
class FPlasticGetBranchesWorker final : public IUnityVersionControlWorker
{
public:
	explicit FPlasticGetBranchesWorker(FUnityVersionControlProvider& InSourceControlProvider)
		: IUnityVersionControlWorker(InSourceControlProvider)
	{}
	virtual ~FPlasticGetBranchesWorker() = default;
	// IUnityVersionControlWorker interface
	virtual FName GetName() const override;
	virtual bool Execute(class FUnityVersionControlCommand& InCommand) override;
	virtual bool UpdateStates() override;

	// Current branch the workspace is on (at the end of the operation)
	FString CurrentBranchName;
};

/** Switch workspace to another branch. */
class FPlasticSwitchToBranchWorker final : public IUnityVersionControlWorker
{
public:
	explicit FPlasticSwitchToBranchWorker(FUnityVersionControlProvider& InSourceControlProvider)
		: IUnityVersionControlWorker(InSourceControlProvider)
	{}
	virtual ~FPlasticSwitchToBranchWorker() = default;
	// IUnityVersionControlWorker interface
	virtual FName GetName() const override;
	virtual bool Execute(class FUnityVersionControlCommand& InCommand) override;
	virtual bool UpdateStates() override;

public:
	/** Temporary states for results */
	TArray<FUnityVersionControlState> States;
};

/** Merge a branch to the current branch. */
class FPlasticMergeBranchWorker final : public IUnityVersionControlWorker
{
public:
	explicit FPlasticMergeBranchWorker(FUnityVersionControlProvider& InSourceControlProvider)
		: IUnityVersionControlWorker(InSourceControlProvider)
	{}
	virtual ~FPlasticMergeBranchWorker() = default;
	// IUnityVersionControlWorker interface
	virtual FName GetName() const override;
	virtual bool Execute(class FUnityVersionControlCommand& InCommand) override;
	virtual bool UpdateStates() override;

public:
	/** Temporary states for results */
	TArray<FUnityVersionControlState> States;
};

/** Create a new child branch. */
class FPlasticCreateBranchWorker final : public IUnityVersionControlWorker
{
public:
	explicit FPlasticCreateBranchWorker(FUnityVersionControlProvider& InSourceControlProvider)
		: IUnityVersionControlWorker(InSourceControlProvider)
	{}
	virtual ~FPlasticCreateBranchWorker() = default;
	// IUnityVersionControlWorker interface
	virtual FName GetName() const override;
	virtual bool Execute(class FUnityVersionControlCommand& InCommand) override;
	virtual bool UpdateStates() override;
};

/** Rename a branch. */
class FPlasticRenameBranchWorker final : public IUnityVersionControlWorker
{
public:
	explicit FPlasticRenameBranchWorker(FUnityVersionControlProvider& InSourceControlProvider)
		: IUnityVersionControlWorker(InSourceControlProvider)
	{}
	virtual ~FPlasticRenameBranchWorker() = default;
	// IUnityVersionControlWorker interface
	virtual FName GetName() const override;
	virtual bool Execute(class FUnityVersionControlCommand& InCommand) override;
	virtual bool UpdateStates() override;
};

/** Delete branches. */
class FPlasticDeleteBranchesWorker final : public IUnityVersionControlWorker
{
public:
	explicit FPlasticDeleteBranchesWorker(FUnityVersionControlProvider& InSourceControlProvider)
		: IUnityVersionControlWorker(InSourceControlProvider)
	{}
	virtual ~FPlasticDeleteBranchesWorker() = default;
	// IUnityVersionControlWorker interface
	virtual FName GetName() const override;
	virtual bool Execute(class FUnityVersionControlCommand& InCommand) override;
	virtual bool UpdateStates() override;
};

/** Plastic update the workspace to latest changes */
class FPlasticSyncWorker final : public IUnityVersionControlWorker
{
public:
	explicit FPlasticSyncWorker(FUnityVersionControlProvider& InSourceControlProvider)
		: IUnityVersionControlWorker(InSourceControlProvider)
	{}
	virtual ~FPlasticSyncWorker() = default;
	// IUnityVersionControlWorker interface
	virtual FName GetName() const override;
	virtual bool Execute(class FUnityVersionControlCommand& InCommand) override;
	virtual bool UpdateStates() override;

public:
	/** Temporary states for results */
	TArray<FUnityVersionControlState> States;
};

/** Get source control status of files on local workspace. */
class FPlasticUpdateStatusWorker final : public IUnityVersionControlWorker
{
public:
	explicit FPlasticUpdateStatusWorker(FUnityVersionControlProvider& InSourceControlProvider)
		: IUnityVersionControlWorker(InSourceControlProvider)
	{}
	virtual ~FPlasticUpdateStatusWorker() = default;
	// IUnityVersionControlWorker interface
	virtual FName GetName() const override;
	virtual bool Execute(class FUnityVersionControlCommand& InCommand) override;
	virtual bool UpdateStates() override;

public:
	/** Temporary states for results */
	TArray<FUnityVersionControlState> States;
};

/** Copy or Move operation on a single file */
class FPlasticCopyWorker final : public IUnityVersionControlWorker
{
public:
	explicit FPlasticCopyWorker(FUnityVersionControlProvider& InSourceControlProvider)
		: IUnityVersionControlWorker(InSourceControlProvider)
	{}
	virtual ~FPlasticCopyWorker() = default;
	// IUnityVersionControlWorker interface
	virtual FName GetName() const override;
	virtual bool Execute(class FUnityVersionControlCommand& InCommand) override;
	virtual bool UpdateStates() override;

public:
	/** Temporary states for results */
	TArray<FUnityVersionControlState> States;
};

/** Plastic command to mark the conflict as solved */
class FPlasticResolveWorker final : public IUnityVersionControlWorker
{
public:
	explicit FPlasticResolveWorker(FUnityVersionControlProvider& InSourceControlProvider)
		: IUnityVersionControlWorker(InSourceControlProvider)
	{}
	virtual ~FPlasticResolveWorker() = default;
	virtual FName GetName() const override;
	virtual bool Execute(class FUnityVersionControlCommand& InCommand) override;
	virtual bool UpdateStates() override;

private:
	/** Temporary states for results */
	TArray<FUnityVersionControlState> States;
};

#if ENGINE_MAJOR_VERSION == 5

class FPlasticGetPendingChangelistsWorker final : public IUnityVersionControlWorker
{
public:
	explicit FPlasticGetPendingChangelistsWorker(FUnityVersionControlProvider& InSourceControlProvider)
		: IUnityVersionControlWorker(InSourceControlProvider)
	{}
	virtual ~FPlasticGetPendingChangelistsWorker() = default;
	// IUnityVersionControlWorker interface
	virtual FName GetName() const override;
	virtual bool Execute(class FUnityVersionControlCommand& InCommand) override;
	virtual bool UpdateStates() override;

public:
	/** Temporary states for results */
	TArray<FUnityVersionControlChangelistState> OutChangelistsStates;
	TArray<TArray<FUnityVersionControlState>> OutCLFilesStates;

private:
	/** Controls whether or not we will remove changelists from the cache after a full update */
	bool bCleanupCache = false;
};

class FPlasticNewChangelistWorker final : public IUnityVersionControlWorker
{
public:
	explicit FPlasticNewChangelistWorker(FUnityVersionControlProvider& InSourceControlProvider);
	virtual ~FPlasticNewChangelistWorker() = default;
	// IUnityVersionControlWorker interface
	virtual FName GetName() const override;
	virtual bool Execute(class FUnityVersionControlCommand& InCommand) override;
	virtual bool UpdateStates() override;

public:
	/** New changelist information */
	FUnityVersionControlChangelist NewChangelist;
	FUnityVersionControlChangelistState NewChangelistState;

	/** Files that were moved */
	TArray<FString> MovedFiles;
};

class FPlasticDeleteChangelistWorker final : public IUnityVersionControlWorker
{
public:
	explicit FPlasticDeleteChangelistWorker(FUnityVersionControlProvider& InSourceControlProvider)
		: IUnityVersionControlWorker(InSourceControlProvider)
	{}
	virtual ~FPlasticDeleteChangelistWorker() = default;
	// IUnityVersionControlWorker interface
	virtual FName GetName() const override;
	virtual bool Execute(class FUnityVersionControlCommand& InCommand) override;
	virtual bool UpdateStates() override;

public:
	FUnityVersionControlChangelist DeletedChangelist;
};

class FPlasticEditChangelistWorker final : public IUnityVersionControlWorker
{
public:
	explicit FPlasticEditChangelistWorker(FUnityVersionControlProvider& InSourceControlProvider)
		: IUnityVersionControlWorker(InSourceControlProvider)
	{}
	virtual ~FPlasticEditChangelistWorker() = default;
	// IUnityVersionControlWorker interface
	virtual FName GetName() const override;
	virtual bool Execute(class FUnityVersionControlCommand& InCommand) override;
	virtual bool UpdateStates() override;

public:
	FUnityVersionControlChangelist EditedChangelist;
	FString EditedDescription;

	/** Reopened files (moved to a new changelist, if any, when editing the Default changelist) */
	TArray<FString> ReopenedFiles;
};

class FPlasticReopenWorker final : public IUnityVersionControlWorker
{
public:
	explicit FPlasticReopenWorker(FUnityVersionControlProvider& InSourceControlProvider)
		: IUnityVersionControlWorker(InSourceControlProvider)
	{}
	virtual ~FPlasticReopenWorker() = default;
	// IUnityVersionControlWorker interface
	virtual FName GetName() const override;
	virtual bool Execute(class FUnityVersionControlCommand& InCommand) override;
	virtual bool UpdateStates() override;

protected:
	/** Reopened files (moved to a new changelist) */
	TArray<FString> ReopenedFiles;

	/** Destination changelist */
	FUnityVersionControlChangelist DestinationChangelist;
};

class FPlasticShelveWorker final : public IUnityVersionControlWorker
{
public:
	explicit FPlasticShelveWorker(FUnityVersionControlProvider& InSourceControlProvider)
		: IUnityVersionControlWorker(InSourceControlProvider)
	{}
	virtual ~FPlasticShelveWorker() = default;

	// IUnityVersionControlWorker interface
	virtual FName GetName() const override;
	virtual bool Execute(FUnityVersionControlCommand& InCommand) override;
	virtual bool UpdateStates() override;

protected:
	int32 ShelveId = ISourceControlState::INVALID_REVISION;

	TArray<FString> ShelvedFiles;

	/** Files that were moved to a new changelist if shelving from the Default Changelist */
	TArray<FString> MovedFiles;

	/** Changelist description if needed */
	FString ChangelistDescription;

	/** Changelist(s) to be updated */
	FUnityVersionControlChangelist InChangelistToUpdate;
	FUnityVersionControlChangelist OutChangelistToUpdate;
};

class FPlasticUnshelveWorker final : public IUnityVersionControlWorker
{
public:
	explicit FPlasticUnshelveWorker(FUnityVersionControlProvider& InSourceControlProvider)
		: IUnityVersionControlWorker(InSourceControlProvider)
	{}
	virtual ~FPlasticUnshelveWorker() = default;

	// IUnityVersionControlWorker interface
	virtual FName GetName() const override;
	virtual bool Execute(FUnityVersionControlCommand& InCommand) override;
	virtual bool UpdateStates() override;

protected:
	/** List of files states after the unshelve */
	TArray<FUnityVersionControlState> States;

	/** Changelist to be updated */
	FUnityVersionControlChangelist ChangelistToUpdate;
};

class FPlasticDeleteShelveWorker final : public IUnityVersionControlWorker
{
public:
	explicit FPlasticDeleteShelveWorker(FUnityVersionControlProvider& InSourceControlProvider)
		: IUnityVersionControlWorker(InSourceControlProvider)
	{}
	virtual ~FPlasticDeleteShelveWorker() = default;

	// IUnityVersionControlWorker interface
	virtual FName GetName() const override;
	virtual bool Execute(FUnityVersionControlCommand& InCommand) override;
	virtual bool UpdateStates() override;

protected:
	/** List of files to remove from shelved files in changelist state */
	TArray<FString> FilesToRemove;

	/** Changelist to be updated */
	FUnityVersionControlChangelist ChangelistToUpdate;

	/** Id of the new shelve (if only a selection of files are deleted from the shelve) */
	int32 ShelveId = ISourceControlState::INVALID_REVISION;
};

#if ENGINE_MINOR_VERSION >= 1

class FPlasticGetChangelistDetailsWorker final : public IUnityVersionControlWorker
{
public:
	FPlasticGetChangelistDetailsWorker(FUnityVersionControlProvider& InSourceControlProvider)
		: IUnityVersionControlWorker(InSourceControlProvider)
	{}
	virtual ~FPlasticGetChangelistDetailsWorker() = default;

	// IUnityVersionControlWorker interface
	virtual FName GetName() const override;
	virtual bool Execute(FUnityVersionControlCommand& InCommand) override;
	virtual bool UpdateStates() override;
};

class FPlasticGetFileWorker final : public IUnityVersionControlWorker
{
public:
	FPlasticGetFileWorker(FUnityVersionControlProvider& InSourceControlProvider)
		: IUnityVersionControlWorker(InSourceControlProvider)
	{}
	virtual ~FPlasticGetFileWorker() = default;

	// IUnityVersionControlWorker interface
	virtual FName GetName() const override;
	virtual bool Execute(FUnityVersionControlCommand& InCommand) override;
	virtual bool UpdateStates() override;
};

#endif

#endif
