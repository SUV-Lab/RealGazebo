using UnrealBuildTool;

public class RealGazeboUI : ModuleRules
{
	public RealGazeboUI(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicIncludePaths.AddRange(new string[] {
			// Add public include paths here
		});
				
		PrivateIncludePaths.AddRange(new string[] {
			// Add private include paths here
		});
			
		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject", 
			"Engine",
			"InputCore",
			"UMG",
			"Slate",
			"SlateCore",
			"RealGazeboBridge"  // Dependency on bridge module for vehicle data
		});
			
		PrivateDependencyModuleNames.AddRange(new string[] {
			"ToolMenus",
			"EditorStyle",
			"UnrealEd",
			"Projects"
		});
		
		DynamicallyLoadedModuleNames.AddRange(new string[] {
			// Add dynamically loaded modules here
		});

		if (Target.bBuildEditor == true)
		{
			PrivateDependencyModuleNames.AddRange(new string[] {
				"EditorStyle",
				"EditorWidgets",
				"UnrealEd",
				"ToolMenus"
			});
		}
	}
}