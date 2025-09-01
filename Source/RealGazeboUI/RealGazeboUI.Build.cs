using UnrealBuildTool;

public class RealGazeboUI : ModuleRules
{
    public RealGazeboUI(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
        bEnableUndefinedIdentifierWarnings = false;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "InputCore",
            "EnhancedInput",
            "UMG",
            "Slate",
            "SlateCore",
            "RealGazebo"
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "GameplayTags",
            "DeveloperSettings"
        });

        if (Target.bBuildEditor)
        {
            PrivateDependencyModuleNames.AddRange(new string[]
            {
                "EditorStyle",
                "ToolMenus"
            });
        }
    }
}