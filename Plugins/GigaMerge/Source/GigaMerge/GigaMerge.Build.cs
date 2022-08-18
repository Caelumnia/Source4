// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class GigaMerge : ModuleRules
{
	public GigaMerge(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

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
				"MergeActors",
				"GigaMesh",
			}
		);
	}
}