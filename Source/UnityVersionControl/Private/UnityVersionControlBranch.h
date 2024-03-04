// Copyright (c) 2024 Unity Technologies

#pragma once

#include "CoreMinimal.h"

class FUnityVersionControlBranch
{
public:
	FString Name;
	FString Repository;
	FString CreatedBy;
	FDateTime Date;
	FString Comment;

	void PopulateSearchString(TArray<FString>& OutStrings) const
	{
		OutStrings.Emplace(Name);
		OutStrings.Emplace(CreatedBy);
		OutStrings.Emplace(Comment);
	}
};

typedef TSharedRef<class FUnityVersionControlBranch, ESPMode::ThreadSafe> FUnityVersionControlBranchRef;
typedef TSharedPtr<class FUnityVersionControlBranch, ESPMode::ThreadSafe> FUnityVersionControlBranchPtr;
