// Copyright (c) 2024 Unity Technologies

#pragma once

#include "CoreMinimal.h"
#include "Runtime/Launch/Resources/Version.h"

#if ENGINE_MAJOR_VERSION == 5

#include "ISourceControlChangelist.h"

/**
 * Unique Identifier of a changelist under source control: a "name" in Unity Version Control
 */
class FUnityVersionControlChangelist : public ISourceControlChangelist
{
public:
	FUnityVersionControlChangelist() = default;

	explicit FUnityVersionControlChangelist(FString&& InChangelistName, const bool bInInitialized = false)
		: ChangelistName(MoveTemp(InChangelistName))
		, bInitialized(bInInitialized)
	{
	}

#if ENGINE_MINOR_VERSION >= 1
	virtual bool CanDelete() const override
	{
		return !IsDefault();
	}
#endif

	bool operator==(const FUnityVersionControlChangelist& InOther) const
	{
		return ChangelistName == InOther.ChangelistName;
	}

	bool operator!=(const FUnityVersionControlChangelist& InOther) const
	{
		return ChangelistName != InOther.ChangelistName;
	}

	virtual bool IsDefault() const /* override NOTE: added in UE5.3 */
	{
		return ChangelistName == DefaultChangelist.ChangelistName;
	}

	void SetInitialized()
	{
		bInitialized = true;
	}

	bool IsInitialized() const
	{
		return bInitialized;
	}

	void Reset()
	{
		ChangelistName.Reset();
		bInitialized = false;
	}

	friend FORCEINLINE uint32 GetTypeHash(const FUnityVersionControlChangelist& InPlasticChangelist)
	{
		return GetTypeHash(InPlasticChangelist.ChangelistName);
	}

	FString GetName() const
	{
		return ChangelistName;
	}

	virtual FString GetIdentifier() const /* override NOTE: added in UE5.3 */
	{
		return ChangelistName;
	}

public:
	static const FUnityVersionControlChangelist DefaultChangelist;

private:
	FString ChangelistName;
	bool bInitialized = false;
};

typedef TSharedRef<class FUnityVersionControlChangelist, ESPMode::ThreadSafe> FUnityVersionControlChangelistRef;

#endif
