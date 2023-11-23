// Copyright (c) 2023 Unity Technologies

#include "Notification.h"

#include "Widgets/Notifications/SNotificationList.h"
#include "Framework/Notifications/NotificationManager.h"

#include "Runtime/Launch/Resources/Version.h"
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
#include "Styling/AppStyle.h"
#else
#include "EditorStyleSet.h"
#endif

#define LOCTEXT_NAMESPACE "UnityVersionControl"

// Display an ongoing notification during the whole operation
void FNotification::DisplayInProgress(const FText& InOperationInProgressString)
{
	if (!OperationInProgress.IsValid())
	{
		FNotificationInfo Info(InOperationInProgressString);
		Info.bFireAndForget = false;
		Info.ExpireDuration = 0.0f;
		Info.FadeOutDuration = 1.0f;
		OperationInProgress = FSlateNotificationManager::Get().AddNotification(Info);
		if (OperationInProgress.IsValid())
		{
			OperationInProgress.Pin()->SetCompletionState(SNotificationItem::CS_Pending);
		}
	}
}

// Remove the ongoing notification at the end of the operation
void FNotification::RemoveInProgress()
{
	if (OperationInProgress.IsValid())
	{
		OperationInProgress.Pin()->ExpireAndFadeout();
		OperationInProgress.Reset();
	}
}

// Display a temporary success notification at the end of the operation
void FNotification::DisplaySuccess(const FName& InOperationName)
{
	const FText NotificationText = FText::Format(
		LOCTEXT("UnityVersionControlOperation_Success", "{0} operation was successful."),
		FText::FromName(InOperationName)
	);

	DisplaySuccess(NotificationText);
}

void FNotification::DisplaySuccess(const FText& InNotificationText)
{
	FNotificationInfo* Info = new FNotificationInfo(InNotificationText);
	Info->ExpireDuration = 1.0f;
	Info->bFireAndForget = true;
	Info->bUseSuccessFailIcons = true;
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
	Info->Image = FAppStyle::GetBrush(TEXT("NotificationList.SuccessImage"));
#else
	Info->Image = FEditorStyle::GetBrush(TEXT("NotificationList.SuccessImage"));
#endif
	FSlateNotificationManager::Get().QueueNotification(Info);
	UE_LOG(LogSourceControl, Verbose, TEXT("%s"), *InNotificationText.ToString());
}

// Display a temporary failure notification at the end of the operation
void FNotification::DisplayFailure(const FName& InOperationName)
{
	const FText NotificationText = FText::Format(
		LOCTEXT("UnityVersionControlOperation_Failure", "Error: {0} operation failed!"),
		FText::FromName(InOperationName)
	);

	DisplayFailure(NotificationText);
}

void FNotification::DisplayFailure(const FText& InNotificationText)
{
	FNotificationInfo* Info = new FNotificationInfo(InNotificationText);
	Info->ExpireDuration = 8.0f;
	Info->bFireAndForget = true;
	FSlateNotificationManager::Get().QueueNotification(Info);
	UE_LOG(LogSourceControl, Error, TEXT("%s"), *InNotificationText.ToString());
}

#undef LOCTEXT_NAMESPACE
