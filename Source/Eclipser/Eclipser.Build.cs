// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Eclipser : ModuleRules
{
	public Eclipser(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
			"Slate",
			"GeometryCore"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { "GeometryFramework" });

		PublicIncludePaths.AddRange(new string[] {
			"Eclipser",
			"Eclipser/Variant_Platforming",
			"Eclipser/Variant_Platforming/Animation",
			"Eclipser/Variant_Combat",
			"Eclipser/Variant_Combat/AI",
			"Eclipser/Variant_Combat/Animation",
			"Eclipser/Variant_Combat/Gameplay",
			"Eclipser/Variant_Combat/Interfaces",
			"Eclipser/Variant_Combat/UI",
			"Eclipser/Variant_SideScrolling",
			"Eclipser/Variant_SideScrolling/AI",
			"Eclipser/Variant_SideScrolling/Gameplay",
			"Eclipser/Variant_SideScrolling/Interfaces",
			"Eclipser/Variant_SideScrolling/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
