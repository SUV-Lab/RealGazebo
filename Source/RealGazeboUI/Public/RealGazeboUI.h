// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

/**
 * RealGazeboUI Module - Provides camera system and UI for RealGazebo plugin
 */
class FRealGazeboUIModule : public IModuleInterface
{
public:
    /** IModuleInterface implementation */
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

    static FRealGazeboUIModule& Get();

private:
    static FRealGazeboUIModule* ModuleInstance;
};