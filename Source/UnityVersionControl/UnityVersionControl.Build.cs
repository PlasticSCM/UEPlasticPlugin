// Copyright (c) 2023 Unity Technologies

using UnrealBuildTool;

public class UnityVersionControl : ModuleRules
{
	public UnityVersionControl(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		// Note: from UE5.4 onward, replaced by IWYUSupport = IWYUSupport.Full; 
		bEnforceIWYU = true;

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

		// NOTE: this produce warnings in SListView Engine code in UE4.27
		UnsafeTypeCastWarningLevel = WarningLevel.Warning;
	}
}
