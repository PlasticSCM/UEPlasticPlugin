// Copyright (c) 2016-2022 Codice Software

// Specialization of classes defined in Engine\Source\Developer\SourceControl\Public\SourceControlOperations.h

#pragma once

#include "CoreMinimal.h"
#include "IPlasticSourceControlWorker.h"
#include "PlasticSourceControlRevision.h"
#include "PlasticSourceControlState.h"
#include "ISourceControlOperation.h"

#include "Runtime/Launch/Resources/Version.h"

#if ENGINE_MAJOR_VERSION == 5
#include "PlasticSourceControlChangelist.h"
#include "PlasticSourceControlChangelistState.h"
#endif

class FPlasticSourceControlProvider;

/**
 * Internal operation used to revert checked-out unchanged files
*/
// NOTE: added to Engine in Unreal Engine 5 for changelists
class FPlasticRevertUnchanged final : public ISourceControlOperation
{
public:
	// ISourceControlOperation interface
	virtual FName GetName() const override;

	virtual FText GetInProgressString() const override;
};


/**
 * Internal operation used to revert checked-out files
*/
class FPlasticRevertAll final : public ISourceControlOperation
{
public:
	// ISourceControlOperation interface
	virtual FName GetName() const override;

	virtual FText GetInProgressString() const override;
};


/**
* Internal operation used to initialize a new Workspace and a new Repository
*/
class FPlasticMakeWorkspace final : public ISourceControlOperation
{
public:
	// ISourceControlOperation interface
	virtual FName GetName() const override;

	virtual FText GetInProgressString() const override;

	FString WorkspaceName;
	FString RepositoryName;
	FString ServerUrl;
};


/** Called when first activated on a project, and then at project load time.
 *  Look for the root directory of the Plastic workspace (where the ".plastic/" subdirectory is located). */
class FPlasticConnectWorker final : public IPlasticSourceControlWorker
{
public:
	explicit FPlasticConnectWorker(FPlasticSourceControlProvider& InSourceControlProvider)
		: IPlasticSourceControlWorker(InSourceControlProvider)
	{}
	virtual ~FPlasticConnectWorker() = default;
	// IPlasticSourceControlWorker interface
	virtual FName GetName() const override;
	virtual bool Execute(class FPlasticSourceControlCommand& InCommand) override;
	virtual bool UpdateStates() override;

public:
	/** Temporary states for results */
	TArray<FPlasticSourceControlState> States;
};

class FPlasticCheckOutWorker final : public IPlasticSourceControlWorker
{
public:
	explicit FPlasticCheckOutWorker(FPlasticSourceControlProvider& InSourceControlProvider)
		: IPlasticSourceControlWorker(InSourceControlProvider)
	{}
	virtual ~FPlasticCheckOutWorker() = default;
	// IPlasticSourceControlWorker interface
	virtual FName GetName() const override;
	virtual bool Execute(class FPlasticSourceControlCommand& InCommand) override;
	virtual bool UpdateStates() override;

public:
	/** Temporary states for results */
	TArray<FPlasticSourceControlState> States;
};

/** Check-in a set of file to the local depot. */
class FPlasticCheckInWorker final : public IPlasticSourceControlWorker
{
public:
	explicit FPlasticCheckInWorker(FPlasticSourceControlProvider& InSourceControlProvider)
		: IPlasticSourceControlWorker(InSourceControlProvider)
	{}
	virtual ~FPlasticCheckInWorker() = default;
	// IPlasticSourceControlWorker interface
	virtual FName GetName() const override;
	virtual bool Execute(class FPlasticSourceControlCommand& InCommand) override;
	virtual bool UpdateStates() override;

public:
	/** Temporary states for results */
	TArray<FPlasticSourceControlState> States;

#if ENGINE_MAJOR_VERSION == 5
	/** Changelist we asked to submit */
	FPlasticSourceControlChangelist InChangelist;
#endif
};

/** Add an untracked file to source control (so only a subset of the Plastic add command). */
class FPlasticMarkForAddWorker final : public IPlasticSourceControlWorker
{
public:
	explicit FPlasticMarkForAddWorker(FPlasticSourceControlProvider& InSourceControlProvider)
		: IPlasticSourceControlWorker(InSourceControlProvider)
	{}
	virtual ~FPlasticMarkForAddWorker() = default;
	// IPlasticSourceControlWorker interface
	virtual FName GetName() const override;
	virtual bool Execute(class FPlasticSourceControlCommand& InCommand) override;
	virtual bool UpdateStates() override;

public:
	/** Temporary states for results */
	TArray<FPlasticSourceControlState> States;
};

/** Delete a file and remove it from source control. */
class FPlasticDeleteWorker final : public IPlasticSourceControlWorker
{
public:
	explicit FPlasticDeleteWorker(FPlasticSourceControlProvider& InSourceControlProvider)
		: IPlasticSourceControlWorker(InSourceControlProvider)
	{}
	virtual ~FPlasticDeleteWorker() = default;
	// IPlasticSourceControlWorker interface
	virtual FName GetName() const override;
	virtual bool Execute(class FPlasticSourceControlCommand& InCommand) override;
	virtual bool UpdateStates() override;

public:
	/** Temporary states for results */
	TArray<FPlasticSourceControlState> States;
};

/** Revert any change to a file to its state on the local depot. */
class FPlasticRevertWorker final : public IPlasticSourceControlWorker
{
public:
	explicit FPlasticRevertWorker(FPlasticSourceControlProvider& InSourceControlProvider)
		: IPlasticSourceControlWorker(InSourceControlProvider)
	{}
	virtual ~FPlasticRevertWorker() = default;
	// IPlasticSourceControlWorker interface
	virtual FName GetName() const override;
	virtual bool Execute(class FPlasticSourceControlCommand& InCommand) override;
	virtual bool UpdateStates() override;

public:
	/** Temporary states for results */
	TArray<FPlasticSourceControlState> States;
};

/** Revert only unchanged file(s) (uncheckout). */
class FPlasticRevertUnchangedWorker final : public IPlasticSourceControlWorker
{
public:
	explicit FPlasticRevertUnchangedWorker(FPlasticSourceControlProvider& InSourceControlProvider)
		: IPlasticSourceControlWorker(InSourceControlProvider)
	{}
	virtual ~FPlasticRevertUnchangedWorker() = default;
	// IPlasticSourceControlWorker interface
	virtual FName GetName() const override;
	virtual bool Execute(class FPlasticSourceControlCommand& InCommand) override;
	virtual bool UpdateStates() override;

public:
	/** Temporary states for results */
	TArray<FPlasticSourceControlState> States;
};

/** Revert all checked-out file(s). */
class FPlasticRevertAllWorker final : public IPlasticSourceControlWorker
{
public:
	explicit FPlasticRevertAllWorker(FPlasticSourceControlProvider& InSourceControlProvider)
		: IPlasticSourceControlWorker(InSourceControlProvider)
	{}
	virtual ~FPlasticRevertAllWorker() = default;
	// IPlasticSourceControlWorker interface
	virtual FName GetName() const override;
	virtual bool Execute(class FPlasticSourceControlCommand& InCommand) override;
	virtual bool UpdateStates() override;

public:
	/** Temporary states for results */
	TArray<FPlasticSourceControlState> States;
};

/** Initialize a new Workspace and a new Repository */
class FPlasticMakeWorkspaceWorker final : public IPlasticSourceControlWorker
{
public:
	explicit FPlasticMakeWorkspaceWorker(FPlasticSourceControlProvider& InSourceControlProvider)
		: IPlasticSourceControlWorker(InSourceControlProvider)
	{}
	virtual ~FPlasticMakeWorkspaceWorker() = default;
	// IPlasticSourceControlWorker interface
	virtual FName GetName() const override;
	virtual bool Execute(class FPlasticSourceControlCommand& InCommand) override;
	virtual bool UpdateStates() override;
};

/** Plastic update the workspace to latest changes */
class FPlasticSyncWorker final : public IPlasticSourceControlWorker
{
public:
	explicit FPlasticSyncWorker(FPlasticSourceControlProvider& InSourceControlProvider)
		: IPlasticSourceControlWorker(InSourceControlProvider)
	{}
	virtual ~FPlasticSyncWorker() = default;
	// IPlasticSourceControlWorker interface
	virtual FName GetName() const override;
	virtual bool Execute(class FPlasticSourceControlCommand& InCommand) override;
	virtual bool UpdateStates() override;

public:
	/** Temporary states for results */
	TArray<FPlasticSourceControlState> States;
};

/** Get source control status of files on local workspace. */
class FPlasticUpdateStatusWorker final : public IPlasticSourceControlWorker
{
public:
	explicit FPlasticUpdateStatusWorker(FPlasticSourceControlProvider& InSourceControlProvider)
		: IPlasticSourceControlWorker(InSourceControlProvider)
	{}
	virtual ~FPlasticUpdateStatusWorker() = default;
	// IPlasticSourceControlWorker interface
	virtual FName GetName() const override;
	virtual bool Execute(class FPlasticSourceControlCommand& InCommand) override;
	virtual bool UpdateStates() override;

public:
	/** Temporary states for results */
	TArray<FPlasticSourceControlState> States;
};

/** Copy or Move operation on a single file */
class FPlasticCopyWorker final : public IPlasticSourceControlWorker
{
public:
	explicit FPlasticCopyWorker(FPlasticSourceControlProvider& InSourceControlProvider)
		: IPlasticSourceControlWorker(InSourceControlProvider)
	{}
	virtual ~FPlasticCopyWorker() = default;
	// IPlasticSourceControlWorker interface
	virtual FName GetName() const override;
	virtual bool Execute(class FPlasticSourceControlCommand& InCommand) override;
	virtual bool UpdateStates() override;

public:
	/** Temporary states for results */
	TArray<FPlasticSourceControlState> States;
};

/** Plastic command to mark the conflict as solved */
class FPlasticResolveWorker final : public IPlasticSourceControlWorker
{
public:
	explicit FPlasticResolveWorker(FPlasticSourceControlProvider& InSourceControlProvider)
		: IPlasticSourceControlWorker(InSourceControlProvider)
	{}
	virtual ~FPlasticResolveWorker() = default;
	virtual FName GetName() const override;
	virtual bool Execute(class FPlasticSourceControlCommand& InCommand) override;
	virtual bool UpdateStates() override;

private:
	/** Temporary states for results */
	TArray<FPlasticSourceControlState> States;
};

#if ENGINE_MAJOR_VERSION == 5

class FPlasticGetPendingChangelistsWorker final : public IPlasticSourceControlWorker
{
public:
	explicit FPlasticGetPendingChangelistsWorker(FPlasticSourceControlProvider& InSourceControlProvider)
		: IPlasticSourceControlWorker(InSourceControlProvider)
	{}
	virtual ~FPlasticGetPendingChangelistsWorker() = default;
	// IPlasticSourceControlWorker interface
	virtual FName GetName() const override;
	virtual bool Execute(class FPlasticSourceControlCommand& InCommand) override;
	virtual bool UpdateStates() override;

public:
	/** Temporary states for results */
	TArray<FPlasticSourceControlChangelistState> OutChangelistsStates;
	TArray<TArray<FPlasticSourceControlState>> OutCLFilesStates;

private:
	/** Controls whether or not we will remove changelists from the cache after a full update */
	bool bCleanupCache = false;
};

class FPlasticNewChangelistWorker final : public IPlasticSourceControlWorker
{
public:
	explicit FPlasticNewChangelistWorker(FPlasticSourceControlProvider& InSourceControlProvider);
	virtual ~FPlasticNewChangelistWorker() = default;
	// IPlasticSourceControlWorker interface
	virtual FName GetName() const override;
	virtual bool Execute(class FPlasticSourceControlCommand& InCommand) override;
	virtual bool UpdateStates() override;

public:
	/** New changelist information */
	FPlasticSourceControlChangelist NewChangelist;
	FPlasticSourceControlChangelistState NewChangelistState;

	/** Files that were moved */
	TArray<FString> MovedFiles;
};

class FPlasticDeleteChangelistWorker final : public IPlasticSourceControlWorker
{
public:
	explicit FPlasticDeleteChangelistWorker(FPlasticSourceControlProvider& InSourceControlProvider)
		: IPlasticSourceControlWorker(InSourceControlProvider)
	{}
	virtual ~FPlasticDeleteChangelistWorker() = default;
	// IPlasticSourceControlWorker interface
	virtual FName GetName() const override;
	virtual bool Execute(class FPlasticSourceControlCommand& InCommand) override;
	virtual bool UpdateStates() override;

public:
	FPlasticSourceControlChangelist DeletedChangelist;
};

class FPlasticEditChangelistWorker final : public IPlasticSourceControlWorker
{
public:
	explicit FPlasticEditChangelistWorker(FPlasticSourceControlProvider& InSourceControlProvider)
		: IPlasticSourceControlWorker(InSourceControlProvider)
	{}
	virtual ~FPlasticEditChangelistWorker() = default;
	// IPlasticSourceControlWorker interface
	virtual FName GetName() const override;
	virtual bool Execute(class FPlasticSourceControlCommand& InCommand) override;
	virtual bool UpdateStates() override;

public:
	FPlasticSourceControlChangelist EditedChangelist;
	FString EditedDescription;

	/** Reopened files (moved to a new changelist, if any, when editing the Default changelist) */
	TArray<FString> ReopenedFiles;
};

class FPlasticReopenWorker final : public IPlasticSourceControlWorker
{
public:
	explicit FPlasticReopenWorker(FPlasticSourceControlProvider& InSourceControlProvider)
		: IPlasticSourceControlWorker(InSourceControlProvider)
	{}
	virtual ~FPlasticReopenWorker() = default;
	// IPlasticSourceControlWorker interface
	virtual FName GetName() const override;
	virtual bool Execute(class FPlasticSourceControlCommand& InCommand) override;
	virtual bool UpdateStates() override;

protected:
	/** Reopened files (moved to a new changelist) */
	TArray<FString> ReopenedFiles;

	/** Destination changelist */
	FPlasticSourceControlChangelist DestinationChangelist;
};

class FPlasticShelveWorker final : public IPlasticSourceControlWorker
{
public:
	explicit FPlasticShelveWorker(FPlasticSourceControlProvider& InSourceControlProvider)
		: IPlasticSourceControlWorker(InSourceControlProvider)
	{}
	virtual ~FPlasticShelveWorker() = default;

	// IPlasticSourceControlWorker interface
	virtual FName GetName() const override;
	virtual bool Execute(FPlasticSourceControlCommand& InCommand) override;
	virtual bool UpdateStates() override;

protected:
	int32 ShelveId = 0;

	TArray<FString> ShelvedFiles;

	/** Reopened files (if shelving from the Default Changelist) */
	TArray<FString> MovedFiles;

	/** Changelist description if needed */
	FString ChangelistDescription;

	/** Changelist(s) to be updated */
	FPlasticSourceControlChangelist InChangelistToUpdate;
	FPlasticSourceControlChangelist OutChangelistToUpdate;
};

class FPlasticDeleteShelveWorker final : public IPlasticSourceControlWorker
{
public:
	explicit FPlasticDeleteShelveWorker(FPlasticSourceControlProvider& InSourceControlProvider)
		: IPlasticSourceControlWorker(InSourceControlProvider)
	{}
	virtual ~FPlasticDeleteShelveWorker() = default;

	// IPlasticSourceControlWorker interface
	virtual FName GetName() const override;
	virtual bool Execute(FPlasticSourceControlCommand& InCommand) override;
	virtual bool UpdateStates() override;

protected:
	/** List of files to remove from shelved files in changelist state */
	TArray<FString> FilesToRemove;

	/** Changelist to be updated */
	FPlasticSourceControlChangelist ChangelistToUpdate;
};

class FPlasticUnshelveWorker : public IPlasticSourceControlWorker
{
public:
	explicit FPlasticUnshelveWorker(FPlasticSourceControlProvider& InSourceControlProvider)
		: IPlasticSourceControlWorker(InSourceControlProvider)
	{}
	virtual ~FPlasticUnshelveWorker() = default;

	// IPlasticSourceControlWorker interface
	virtual FName GetName() const override;
	virtual bool Execute(FPlasticSourceControlCommand& InCommand) override;
	virtual bool UpdateStates() override;

protected:
	/** Changelist to be updated */
	FPlasticSourceControlChangelist ChangelistToUpdate;

	/** List of files states after update */
	TArray<FPlasticSourceControlState> ChangelistFilesStates;
};

#endif
