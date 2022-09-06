// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class GigaMesh : ModuleRules
{
	public GigaMesh(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"RHI",
				"RenderCore",
			}
		);
	}
}