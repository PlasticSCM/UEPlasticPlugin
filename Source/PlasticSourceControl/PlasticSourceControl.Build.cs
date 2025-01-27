// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class PlasticSourceControl : ModuleRules
{
	public PlasticSourceControl(ReadOnlyTargetRules Target) : base(Target)
	{
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				"InputCore",
				"UnrealEd",
				"LevelEditor",
				"DesktopPlatform",
				"SourceControl",
				"SourceControlWindows",
				"XmlParser",
				"Projects",
				"AssetRegistry",
				"DeveloperSettings",
				"ToolMenus",
				"ContentBrowser",
			}
		);

		CppCompileWarningSettings.UnsafeTypeCastWarningLevel = WarningLevel.Error;
	}
}
