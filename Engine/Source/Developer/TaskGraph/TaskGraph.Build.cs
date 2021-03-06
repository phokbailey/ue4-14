// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class TaskGraph : ModuleRules
{
	public TaskGraph(TargetInfo Target)
	{
		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"Core",
				"CoreUObject",
				"Slate",
				"SlateCore",
                "EditorStyle",
				"Engine",
                "InputCore"
			}
		);
	}
}
