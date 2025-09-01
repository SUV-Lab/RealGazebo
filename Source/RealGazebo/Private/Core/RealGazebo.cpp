// Copyright Epic Games, Inc. All Rights Reserved.

#include "Core/RealGazebo.h"
#include "Core.h"
#include "Modules/ModuleManager.h"

#define LOCTEXT_NAMESPACE "FRealGazeboModule"

FRealGazeboModule* FRealGazeboModule::ModuleInstance = nullptr;

void FRealGazeboModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
    ModuleInstance = this;
    
    UE_LOG(LogTemp, Display, TEXT("====== RealGazeboModule startup ======"));
}

void FRealGazeboModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.

    ModuleInstance = nullptr;
    
    UE_LOG(LogTemp, Log, TEXT("RealGazeboModule shutdown"));
}

FRealGazeboModule& FRealGazeboModule::Get()
{
    return FModuleManager::LoadModuleChecked<FRealGazeboModule>("RealGazebo");
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FRealGazeboModule, RealGazebo)