// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class AutoBatch : ModuleRules
{
	public AutoBatch(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PrivateIncludePaths.AddRange(
			new string[]
			{
				
			}
		);
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"Engine",
				"Slate",
				"SlateCore",
				"UnrealEd",
				"EditorStyle"
			}
		);
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				
			}
		);
	}
}