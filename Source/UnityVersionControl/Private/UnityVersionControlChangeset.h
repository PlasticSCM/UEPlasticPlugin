// Copyright (c) 2025 Unity Technologies

#pragma once

#include "CoreMinimal.h"

#include "UnityVersionControlState.h"

class FUnityVersionControlChangeset
{
public:
	int32 ChangesetId;
	FString CreatedBy;
	FDateTime Date;
	FString Comment;
	FString Branch;
	// Note: array of file States, each with one Revision for Diffing (like for Files and ShelvedFiles in FUnityVersionControlChangelist)
	TArray<FUnityVersionControlStateRef> Files;

	void PopulateSearchString(TArray<FString>& OutStrings) const
	{
		OutStrings.Emplace(CreatedBy);
		OutStrings.Emplace(Comment);
		OutStrings.Emplace(Branch);
	}
};

typedef TSharedRef<class FUnityVersionControlChangeset, ESPMode::ThreadSafe> FUnityVersionControlChangesetRef;
typedef TSharedPtr<class FUnityVersionControlChangeset, ESPMode::ThreadSafe> FUnityVersionControlChangesetPtr;
