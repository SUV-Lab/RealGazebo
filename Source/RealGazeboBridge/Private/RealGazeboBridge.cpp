
#include "RealGazeboBridge.h"
#include "Engine/Engine.h"

DEFINE_LOG_CATEGORY(LogRealGazeboBridge);

#define LOCTEXT_NAMESPACE "FRealGazeboBridgeModule"

FRealGazeboBridgeModule* FRealGazeboBridgeModule::ModuleInstance = nullptr;

void FRealGazeboBridgeModule::StartupModule()
{
    ModuleInstance = this;
    
    UE_LOG(LogRealGazeboBridge, Display, TEXT("====== RealGazeboBridge Module Startup ======"));
    UE_LOG(LogRealGazeboBridge, Display, TEXT("Subsystem architecture ready"));
    UE_LOG(LogRealGazeboBridge, Display, TEXT("Object pooling system available"));
    UE_LOG(LogRealGazeboBridge, Display, TEXT("Batch processing enabled"));
}

void FRealGazeboBridgeModule::ShutdownModule()
{
    UE_LOG(LogRealGazeboBridge, Log, TEXT("RealGazeboBridge Module shutdown"));
    ModuleInstance = nullptr;
}

FRealGazeboBridgeModule& FRealGazeboBridgeModule::Get()
{
    return FModuleManager::LoadModuleChecked<FRealGazeboBridgeModule>("RealGazeboBridge");
}

bool FRealGazeboBridgeModule::IsAvailable()
{
    return FModuleManager::Get().IsModuleLoaded("RealGazeboBridge");
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FRealGazeboBridgeModule, RealGazeboBridge)