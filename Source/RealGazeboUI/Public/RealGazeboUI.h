#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

DECLARE_LOG_CATEGORY_EXTERN(LogRealGazeboUI, Log, All);

/**
 * RealGazebo UI Module
 * Provides UI components for vehicle data visualization and management
 */
class FRealGazeboUIModule : public IModuleInterface
{
public:
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	/** Module singleton access */
	static FRealGazeboUIModule& Get()
	{
		return FModuleManager::LoadModuleChecked<FRealGazeboUIModule>("RealGazeboUI");
	}

	/** Check if module is loaded */
	static bool IsAvailable()
	{
		return FModuleManager::Get().IsModuleLoaded("RealGazeboUI");
	}

private:
	/** Handle module startup */
	void RegisterUIComponents();
	
	/** Handle module shutdown */
	void UnregisterUIComponents();
};