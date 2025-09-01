#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"

DECLARE_LOG_CATEGORY_EXTERN(LogRealGazeboStreaming, Log, All);

class FRealGazeboStreamingModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
    
    static FRealGazeboStreamingModule& Get()
    {
        return FModuleManager::LoadModuleChecked<FRealGazeboStreamingModule>("RealGazeboStreaming");
    }
    
    static bool IsAvailable()
    {
        return FModuleManager::Get().IsModuleLoaded("RealGazeboStreaming");
    }
};