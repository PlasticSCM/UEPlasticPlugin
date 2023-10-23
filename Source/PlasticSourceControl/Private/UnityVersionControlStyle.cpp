// Copyright (c) 2023 Unity Technologies

#include "UnityVersionControlStyle.h"

#include "Framework/Application/SlateApplication.h"
#include "Interfaces/IPluginManager.h"
#include "Slate/SlateGameResources.h"
#include "Styling/SlateStyleMacros.h"
#include "Styling/SlateStyleRegistry.h"

#include "PlasticSourceControlModule.h"

#define RootToContentDir Style->RootToContentDir

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
const FVector2D Icon20x20(20.0f, 20.0f);

TSharedRef< FSlateStyleSet > FUnityVersionControlStyle::Create()
{
	TSharedRef< FSlateStyleSet > Style = MakeShareable(new FSlateStyleSet("UnityVersionControlStyle"));
	Style->SetContentRoot(FPlasticSourceControlModule::GetPlugin()->GetBaseDir() / TEXT("Resources"));

	Style->Set("UnityVersionControl.PluginIcon.Small", new FSlateImageBrush(FUnityVersionControlStyle::InContent("Icon128", ".png"), Icon16x16));
	// Note: show how to add SVG icons to the plugin
//	Style->Set("UnityVersionControl.PlaceholderButtonIcon", new IMAGE_BRUSH_SVG(TEXT("PlaceholderButtonIcon"), Icon20x20));
//	Style->Set("UnityVersionControl.PlaceholderButtonIcon.Small", new IMAGE_BRUSH_SVG(TEXT("PlaceholderButtonIcon"), Icon16x16));

	return Style;
}

FString FUnityVersionControlStyle::InContent(const FString& RelativePath, const ANSICHAR* Extension)
{
	auto myself = FPlasticSourceControlModule::GetPlugin();
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
