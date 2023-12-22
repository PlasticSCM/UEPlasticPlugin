// Copyright (c) 2023 Unity Technologies

#if ENGINE_MAJOR_VERSION == 5

#include "UnityVersionControlChangelistState.h"

#define LOCTEXT_NAMESPACE "UnityVersionControl.ChangelistState"

FName FUnityVersionControlChangelistState::GetIconName() const
{
	// Mimic P4V colors, returning the red icon if there are active file(s), the blue if the changelist is empty or all the files are shelved.
	return Files.Num() > 0 ? FName("SourceControl.Changelist") : FName("SourceControl.ShelvedCHangelist");
}

FName FUnityVersionControlChangelistState::GetSmallIconName() const
{
	return GetIconName();
}

FText FUnityVersionControlChangelistState::GetDisplayText() const
{
	return FText::FromString(Changelist.GetName());
}

FText FUnityVersionControlChangelistState::GetDescriptionText() const
{
	return FText::FromString(Description);
}

FText FUnityVersionControlChangelistState::GetDisplayTooltip() const
{
	return LOCTEXT("Tooltip", "Tooltip");
}

const FDateTime& FUnityVersionControlChangelistState::GetTimeStamp() const
{
	return TimeStamp;
}

#if ENGINE_MINOR_VERSION >= 4

const TArray<FSourceControlStateRef> FUnityVersionControlChangelistState::GetFilesStates() const
{
	return Files;
}

int32 FUnityVersionControlChangelistState::GetFilesStatesNum() const
{
	return Files.Num();
}

const TArray<FSourceControlStateRef> FUnityVersionControlChangelistState::GetShelvedFilesStates() const
{
	return ShelvedFiles;
}

int32 FUnityVersionControlChangelistState::GetShelvedFilesStatesNum() const
{
	return ShelvedFiles.Num();
}

#else // ENGINE_MINOR_VERSION < 4

const TArray<FSourceControlStateRef>& FUnityVersionControlChangelistState::GetFilesStates() const
{
	return Files;
}

const TArray<FSourceControlStateRef>& FUnityVersionControlChangelistState::GetShelvedFilesStates() const
{
	return ShelvedFiles;
}

#endif // ENGINE_MINOR_VERSION

FSourceControlChangelistRef FUnityVersionControlChangelistState::GetChangelist() const
{
	FUnityVersionControlChangelistRef ChangelistCopy = MakeShareable( new FUnityVersionControlChangelist(Changelist));
	return StaticCastSharedRef<ISourceControlChangelist>(ChangelistCopy);
}

#undef LOCTEXT_NAMESPACE

#endif
