using UnrealBuildTool;

public class RealGazeboUI : ModuleRules
{
	public RealGazeboUI(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		// Public include paths for external access
		PublicIncludePaths.AddRange(new string[] {
			"RealGazeboUI/Public",
			"RealGazeboUI/Public/Widgets",
			"RealGazeboUI/Public/Data",
		});
				
		// Private include paths for internal implementation
		PrivateIncludePaths.AddRange(new string[] {
			"RealGazeboUI/Private",
			"RealGazeboUI/Private/Widgets",
			"RealGazeboUI/Private/Data",
		});
			
		// Core runtime dependencies
		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"UMG",
			"Slate",
			"SlateCore",
			"HTTP",
			"Json",
			"JsonUtilities",
			"RealGazeboBridge"
		});
			
		// Private implementation dependencies
		PrivateDependencyModuleNames.AddRange(new string[] {
			"ApplicationCore",
			"RenderCore",
			"RHI"
		});
		
		// Editor-specific dependencies
		if (Target.bBuildEditor)
		{
			PublicDependencyModuleNames.AddRange(new string[] {
				"EditorStyle",
				"EditorWidgets",
				"UnrealEd",
				"ToolMenus",
				"WorkspaceMenuStructure",
				"PropertyEditor",
				"BlueprintGraph",
				"KismetCompiler",
				"DesktopPlatform"
			});
			
			PrivateDependencyModuleNames.AddRange(new string[] {
				"DesktopWidgets",
				"GraphEditor",
				"Kismet",
				"KismetWidgets",
				"AssetTools",
				"ContentBrowser",
				"LevelEditor",
				"MainFrame",
				"Projects",
				"Persona",
				"AnimGraph"
			});
		}
		
		// Platform-specific configurations
		if (Target.Platform == UnrealTargetPlatform.Linux)
		{
			PrivateDependencyModuleNames.AddRange(new string[] {
				"UnixCommonStartup"
			});
		}
		else if (Target.Platform == UnrealTargetPlatform.Win64)
		{
			PrivateDependencyModuleNames.AddRange(new string[] {
				"D3D12RHI",
				"D3D11RHI"
			});
		}
		
		DynamicallyLoadedModuleNames.AddRange(new string[] {
			// Reserved for future dynamic module loading
		});
		
		// Optimization settings
		OptimizeCode = CodeOptimization.InShippingBuildsOnly;
		
		// Define preprocessor macros
		PublicDefinitions.AddRange(new string[] {
			"REALGAZEBO_UI_MODULE=1"
		});
		
		if (Target.Configuration == UnrealTargetConfiguration.Debug ||
			Target.Configuration == UnrealTargetConfiguration.DebugGame)
		{
			PublicDefinitions.Add("REALGAZEBO_UI_DEBUG=1");
		}
	}
}