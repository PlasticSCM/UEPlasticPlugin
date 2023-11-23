// Copyright (c) 2023 Unity Technologies

#include "UnityVersionControlStyle.h"

#include "Framework/Application/SlateApplication.h"
#include "Interfaces/IPluginManager.h"
#include "Slate/SlateGameResources.h"
#if ENGINE_MAJOR_VERSION >= 5
#include "Styling/SlateStyleMacros.h"
#endif
#include "Styling/SlateStyleRegistry.h"

#include "UnityVersionControlModule.h"

TSharedPtr<FSlateStyleSet> FUnityVersionControlStyle::StyleInstance = nullptr;

void FUnityVersionControlStyle::Initialize()
{
	if (!StyleInstance.IsValid())
	{
		StyleInstance = Create();
		FSlateStyleRegistry::RegisterSlateStyle(*StyleInstance);
	}
}

void FUnityVersionControlStyle::Shutdown()
{
	FSlateStyleRegistry::UnRegisterSlateStyle(*StyleInstance);
	ensure(StyleInstance.IsUnique());
	StyleInstance.Reset();
}

FName FUnityVersionControlStyle::GetStyleSetName()
{
	static FName StyleSetName(TEXT("UnityVersionControlStyle"));
	return StyleSetName;
}

const FVector2D Icon16x16(16.0f, 16.0f);

TSharedRef<FSlateStyleSet> FUnityVersionControlStyle::Create()
{
	TSharedRef<FSlateStyleSet> Style = MakeShareable(new FSlateStyleSet("UnityVersionControlStyle"));
	Style->SetContentRoot(FUnityVersionControlModule::GetPlugin()->GetBaseDir() / TEXT("Resources"));

	Style->Set("UnityVersionControl.PluginIcon.Small", new FSlateImageBrush(FUnityVersionControlStyle::InContent("Icon128", ".png"), Icon16x16));

	return Style;
}

FString FUnityVersionControlStyle::InContent(const FString& RelativePath, const ANSICHAR* Extension)
{
	auto myself = FUnityVersionControlModule::GetPlugin();
	check(myself.IsValid());
	static FString ContentDir = myself->GetBaseDir() / TEXT("Resources");
	return (ContentDir / RelativePath) + Extension;
}

void FUnityVersionControlStyle::ReloadTextures()
{
	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().GetRenderer()->ReloadTextureResources();
	}
}

const ISlateStyle& FUnityVersionControlStyle::Get()
{
	return *StyleInstance;
}
