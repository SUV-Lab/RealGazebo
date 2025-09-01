// Copyright Epic Games, Inc. All Rights Reserved.

#include "RealGazeboUI.h"

IMPLEMENT_MODULE(FRealGazeboUIModule, RealGazeboUI)

FRealGazeboUIModule* FRealGazeboUIModule::ModuleInstance = nullptr;

void FRealGazeboUIModule::StartupModule()
{
    ModuleInstance = this;
    UE_LOG(LogTemp, Log, TEXT("RealGazeboUI module started"));
}

void FRealGazeboUIModule::ShutdownModule()
{
    UE_LOG(LogTemp, Log, TEXT("RealGazeboUI module shutdown"));
    ModuleInstance = nullptr;
}

FRealGazeboUIModule& FRealGazeboUIModule::Get()
{
    check(ModuleInstance);
    return *ModuleInstance;
}