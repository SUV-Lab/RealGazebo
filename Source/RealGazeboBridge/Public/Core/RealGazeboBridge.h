
#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

DECLARE_LOG_CATEGORY_EXTERN(LogRealGazeboBridge, Log, All);

/**
 * RealGazebo Bridge Module - PX4-Gazebo simulation bridge
 * 
 * Key Features:
 * - Subsystem-based architecture
 * - Object pooling for memory management
 * - Batch processing for network data
 * - Simple rendering (all vehicles visible like original RealGazebo)
 */
class REALGAZEBOBRIDGE_API FRealGazeboBridgeModule : public IModuleInterface
{
public:
    /** IModuleInterface implementation */
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

    /** Get the module instance */
    static FRealGazeboBridgeModule& Get();

    /** Check if the module is loaded */
    static bool IsAvailable();

private:
    /** Module instance for singleton access */
    static FRealGazeboBridgeModule* ModuleInstance;
};