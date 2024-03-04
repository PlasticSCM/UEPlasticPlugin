// Copyright (c) 2024 Unity Technologies

#include "UnityVersionControlSettings.h"
#include "UnityVersionControlModule.h"
#include "UnityVersionControlProvider.h"
#include "UnityVersionControlUtils.h"
#include "SourceControlHelpers.h"

#include "Misc/ScopeLock.h"
#include "Misc/ConfigCacheIni.h"
#include "HAL/PlatformTime.h"

namespace PlasticSettingsConstants
{

/** The section of the ini file we load our settings from */
static const FString SettingsSection = TEXT("UnityVersionControl.UnityVersionControlSettings");

}

FString FUnityVersionControlSettings::GetBinaryPath() const
{
	FScopeLock ScopeLock(&CriticalSection);
	return BinaryPath;
}

bool FUnityVersionControlSettings::SetBinaryPath(const FString& InString)
{
	FScopeLock ScopeLock(&CriticalSection);
	const bool bChanged = (BinaryPath != InString);
	if (bChanged)
	{
		BinaryPath = InString;
	}
	return bChanged;
}

bool FUnityVersionControlSettings::GetUpdateStatusAtStartup() const
{
	FScopeLock ScopeLock(&CriticalSection);
	return bUpdateStatusAtStartup;
}

void FUnityVersionControlSettings::SetUpdateStatusAtStartup(const bool bInUpdateStatusAtStartup)
{
	FScopeLock ScopeLock(&CriticalSection);
	bUpdateStatusAtStartup = bInUpdateStatusAtStartup;
}

bool FUnityVersionControlSettings::GetUpdateStatusOtherBranches() const
{
	FScopeLock ScopeLock(&CriticalSection);
	return bUpdateStatusOtherBranches;
}

void FUnityVersionControlSettings::SetUpdateStatusOtherBranches(const bool bInUpdateStatusOtherBranches)
{
	FScopeLock ScopeLock(&CriticalSection);
	bUpdateStatusOtherBranches = bInUpdateStatusOtherBranches;
}

bool FUnityVersionControlSettings::GetEnableVerboseLogs() const
{
	FScopeLock ScopeLock(&CriticalSection);
	return bEnableVerboseLogs;
}

void FUnityVersionControlSettings::SetEnableVerboseLogs(const bool bInEnableVerboseLogs)
{
	FScopeLock ScopeLock(&CriticalSection);
	bEnableVerboseLogs = bInEnableVerboseLogs;
}

// This is called at startup nearly before anything else in our module: BinaryPath will then be used by the provider
void FUnityVersionControlSettings::LoadSettings()
{
	FScopeLock ScopeLock(&CriticalSection);
	const FString& IniFile = SourceControlHelpers::GetSettingsIni();
	GConfig->GetString(*PlasticSettingsConstants::SettingsSection, TEXT("BinaryPath"), BinaryPath, IniFile);
	GConfig->GetBool(*PlasticSettingsConstants::SettingsSection, TEXT("UpdateStatusAtStartup"), bUpdateStatusAtStartup, IniFile);
	GConfig->GetBool(*PlasticSettingsConstants::SettingsSection, TEXT("UpdateStatusOtherBranches"), bUpdateStatusOtherBranches, IniFile);
	GConfig->GetBool(*PlasticSettingsConstants::SettingsSection, TEXT("EnableVerboseLogs"), bEnableVerboseLogs, IniFile);
}

void FUnityVersionControlSettings::SaveSettings() const
{
	FScopeLock ScopeLock(&CriticalSection);
	const FString& IniFile = SourceControlHelpers::GetSettingsIni();
	GConfig->SetString(*PlasticSettingsConstants::SettingsSection, TEXT("BinaryPath"), *BinaryPath, IniFile);
	GConfig->SetBool(*PlasticSettingsConstants::SettingsSection, TEXT("UpdateStatusAtStartup"), bUpdateStatusAtStartup, IniFile);
	GConfig->SetBool(*PlasticSettingsConstants::SettingsSection, TEXT("UpdateStatusOtherBranches"), bUpdateStatusOtherBranches, IniFile);
	GConfig->SetBool(*PlasticSettingsConstants::SettingsSection, TEXT("EnableVerboseLogs"), bEnableVerboseLogs, IniFile);
}
