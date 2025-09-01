#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"

DECLARE_LOG_CATEGORY_EXTERN(LogRealGazeboComm, Log, All);

class FRealGazeboCommModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
    
    static FRealGazeboCommModule& Get()
    {
        return FModuleManager::LoadModuleChecked<FRealGazeboCommModule>("RealGazeboComm");
    }
    
    static bool IsAvailable()
    {
        return FModuleManager::Get().IsModuleLoaded("RealGazeboComm");
    }
};