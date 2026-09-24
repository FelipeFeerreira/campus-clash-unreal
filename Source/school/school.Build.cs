// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class school : ModuleRules
{
	public school(ReadOnlyTargetRules Target) : base(Target)
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
			"SlateCore",
			"Niagara",
			"PhysicsCore"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"school",
			"school/Variant_Platforming",
			"school/Variant_Platforming/Animation",
			"school/Variant_Combat",
			"school/Variant_Combat/AI",
			"school/Variant_Combat/Animation",
			"school/Variant_Combat/Gameplay",
			"school/Variant_Combat/Interfaces",
			"school/Variant_Combat/UI",
			"school/Variant_SideScrolling",
			"school/Variant_SideScrolling/AI",
			"school/Variant_SideScrolling/Gameplay",
			"school/Variant_SideScrolling/Interfaces",
			"school/Variant_SideScrolling/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
