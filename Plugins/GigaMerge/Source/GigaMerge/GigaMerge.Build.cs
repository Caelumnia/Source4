// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class GigaMerge : ModuleRules
{
	public GigaMerge(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PrivateIncludePaths.AddRange(
			new string[] { }
		);
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"MergeActors",
			}
		);
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"InputCore",
				"Slate",
				"SlateCore",
				"EditorStyle",
				"UnrealEd",
			}
		);
	}
}