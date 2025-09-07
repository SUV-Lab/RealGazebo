#include "RealGazeboUI.h"
#include "Core/RealGazeboUISubsystem.h"

#define LOCTEXT_NAMESPACE "FRealGazeboUIModule"

DEFINE_LOG_CATEGORY(LogRealGazeboUI);

void FRealGazeboUIModule::StartupModule()
{
	UE_LOG(LogRealGazeboUI, Display, TEXT("RealGazeboUI Module: Starting up"));
	
	// Additional startup code can be added here
}

void FRealGazeboUIModule::ShutdownModule()
{
	UE_LOG(LogRealGazeboUI, Display, TEXT("RealGazeboUI Module: Shutting down"));
	
	// Additional cleanup code can be added here
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FRealGazeboUIModule, RealGazeboUI)