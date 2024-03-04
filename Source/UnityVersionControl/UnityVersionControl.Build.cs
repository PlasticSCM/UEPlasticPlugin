// Copyright (c) 2024 Unity Technologies

using UnrealBuildTool;

public class UnityVersionControl : ModuleRules
{
	public UnityVersionControl(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Slate",
				"SlateCore",
				"InputCore",
				"EditorStyle",
				"UnrealEd",
				"LevelEditor",
				"SourceControl",
				"XmlParser",
				"Projects",
				"AssetRegistry",
				"DeveloperSettings",
				"ToolMenus",
				"ContentBrowser",
			}
		);
	}
}
