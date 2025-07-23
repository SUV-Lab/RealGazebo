// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FRTSPStreamerThread;
struct FRTSPStreamSettings;

class FRealGazeboModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	static FRealGazeboModule& Get();

    void RegisterStream(const FString& StreamPath, const FRTSPStreamSettings& Settings);
    void UnregisterStream(const FString& StreamPath);
    void UpdateStream(const FString& StreamPath, const TArray<uint8>& FrameData);
    bool IsServerRunning() const;

    const TUniquePtr<FRTSPStreamerThread>& GetStreamerThread() const { return StreamerThread; }

private:
    TUniquePtr<FRTSPStreamerThread> StreamerThread;
    static FRealGazeboModule* ModuleInstance;
};
