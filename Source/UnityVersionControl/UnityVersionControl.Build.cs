// Copyright (c) 2025 Unity Technologies

using UnrealBuildTool;

public class UnityVersionControl : ModuleRules
{
	public UnityVersionControl(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		// Note: from UE5.2 onward, bEnforceIWYU = true; is replaced by IWYUSupport = IWYUSupport.Full;
		bEnforceIWYU = true;
		// IWYUSupport = IWYUSupport.Full;

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				"InputCore",
				"EditorStyle",
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
	}
}
