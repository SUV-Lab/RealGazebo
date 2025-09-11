#include "RealGazeboUI.h"
#include "Engine/Engine.h"

#define LOCTEXT_NAMESPACE "FRealGazeboUIModule"

DEFINE_LOG_CATEGORY(LogRealGazeboUI);

void FRealGazeboUIModule::StartupModule()
{
	UE_LOG(LogRealGazeboUI, Log, TEXT("RealGazeboUI module starting up"));

	// Register UI components
	RegisterUIComponents();

	UE_LOG(LogRealGazeboUI, Log, TEXT("RealGazeboUI module started successfully"));
}

void FRealGazeboUIModule::ShutdownModule()
{
	UE_LOG(LogRealGazeboUI, Log, TEXT("RealGazeboUI module shutting down"));

	// Unregister UI components
	UnregisterUIComponents();

	UE_LOG(LogRealGazeboUI, Log, TEXT("RealGazeboUI module shut down"));
}

void FRealGazeboUIModule::RegisterUIComponents()
{
	// Register custom widget classes for Blueprint use
	// This ensures our widgets are available in Blueprint editor

	UE_LOG(LogRealGazeboUI, Verbose, TEXT("Registering UI components"));

	// Note: Widget classes are automatically registered through their UCLASS macros
	// Additional registration logic can be added here if needed
}

void FRealGazeboUIModule::UnregisterUIComponents()
{
	// Cleanup any registered components
	UE_LOG(LogRealGazeboUI, Verbose, TEXT("Unregistering UI components"));

	// Cleanup logic can be added here if needed
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FRealGazeboUIModule, RealGazeboUI)