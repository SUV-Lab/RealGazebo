#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "StreamingTypes.h"
#include "AdvancedCaptureManager.generated.h"

// Forward declarations
class UVehicleCameraStreamComponent;

/**
 * Advanced capture manager for optimizing frame capture across multiple cameras
 * 
 * Manages high-performance capture operations, frame buffering, and
 * coordination between multiple camera streams for optimal performance.
 */
UCLASS(BlueprintType)
class REALGAZEBOSTREAMING_API UAdvancedCaptureManager : public UObject
{
    GENERATED_BODY()

public:
    UAdvancedCaptureManager();

    //----------------------------------------------------------
    // Capture Management
    //----------------------------------------------------------

    /** Initialize capture system */
    UFUNCTION(BlueprintCallable, Category = "Capture Manager")
    bool Initialize();

    /** Shutdown capture system */
    UFUNCTION(BlueprintCallable, Category = "Capture Manager")
    void Shutdown();

    /** Register camera for managed capture */
    UFUNCTION(BlueprintCallable, Category = "Capture Manager")
    void RegisterCamera(UVehicleCameraStreamComponent* Camera);

    /** Unregister camera from managed capture */
    UFUNCTION(BlueprintCallable, Category = "Capture Manager")
    void UnregisterCamera(UVehicleCameraStreamComponent* Camera);

    /** Get capture performance statistics */
    UFUNCTION(BlueprintCallable, Category = "Capture Manager")
    void GetCaptureStats(int32& OutActiveCameras, float& OutAverageFPS, float& OutMemoryUsageMB) const;

protected:
    //----------------------------------------------------------
    // Internal Management
    //----------------------------------------------------------

    /** Registered cameras for capture management */
    UPROPERTY()
    TArray<TObjectPtr<UVehicleCameraStreamComponent>> RegisteredCameras;

    /** Is capture system initialized */
    bool bIsInitialized = false;

    /** Performance tracking */
    mutable int32 TotalFramesCaptured = 0;
    mutable float LastPerformanceUpdate = 0.0f;
    mutable float CachedAverageFPS = 0.0f;

    //----------------------------------------------------------
    // Performance Optimization
    //----------------------------------------------------------

    /** Optimize capture operations across cameras */
    void OptimizeCapturePerformance();

    /** Update performance statistics */
    void UpdatePerformanceStats() const;
};