// Copyright (c) 2024 Unity Technologies

#pragma once

#include "CoreMinimal.h"

#include "ISourceControlState.h"

class FUnityVersionControlLock
{
public:
	int32 ItemId = ISourceControlState::INVALID_REVISION;
	FString Path;
	FString Status;
	bool bIsLocked = false;
	FDateTime Date;
	FString Owner;
	FString DestinationBranch;
	FString Branch;
	FString Workspace;

	void PopulateSearchString(TArray<FString>& OutStrings) const
	{
		OutStrings.Emplace(Path);
		OutStrings.Emplace(Owner);
		OutStrings.Emplace(Branch);
		OutStrings.Emplace(Workspace);
	}
};

typedef TSharedRef<class FUnityVersionControlLock, ESPMode::ThreadSafe> FUnityVersionControlLockRef;
typedef TSharedPtr<class FUnityVersionControlLock, ESPMode::ThreadSafe> FUnityVersionControlLockPtr;
