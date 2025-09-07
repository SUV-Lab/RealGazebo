#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Core/UIVehicleDataInterface.h"
#include "Camera/RealGazeboCameraTypes.h"
#include "DirectWidgetSetup.generated.h"

/**
 * Direct widget setup - inherit from this instead of RealGazeboMainWidget
 * This handles everything automatically without requiring Blueprint setup
 */
UCLASS(BlueprintType, Blueprintable)
class REALGAZEBOUI_API UDirectWidgetSetup : public UUserWidget
{
    GENERATED_BODY()

public:
    UDirectWidgetSetup(const FObjectInitializer& ObjectInitializer);

    //----------------------------------------------------------
    // Widget Interface - Handles everything automatically
    //----------------------------------------------------------

    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

    //----------------------------------------------------------
    // Simple Functions for Blueprint (Optional)
    //----------------------------------------------------------

    /** Get vehicle count for display */
    UFUNCTION(BlueprintCallable, Category = "Direct Widget Setup")
    int32 GetVehicleCount() const;

    /** Get vehicle data for display */
    UFUNCTION(BlueprintCallable, Category = "Direct Widget Setup")
    TArray<FUIVehicleData> GetVehicleData() const;

    /** Manually refresh data */
    UFUNCTION(BlueprintCallable, Category = "Direct Widget Setup")
    void RefreshData();

    /** Switch camera mode */
    UFUNCTION(BlueprintCallable, Category = "Direct Widget Setup")
    void SwitchCameraMode(ERealGazeboCameraMode NewMode);

    /** Get current camera mode */
    UFUNCTION(BlueprintCallable, Category = "Direct Widget Setup")
    ERealGazeboCameraMode GetCurrentCameraMode() const;

    /** Test if everything is working */
    UFUNCTION(BlueprintCallable, Category = "Direct Widget Setup")
    FString TestConnection() const;

    //----------------------------------------------------------
    // Settings (can be set in Blueprint)
    //----------------------------------------------------------

    /** Update frequency */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Direct Widget Setup")
    float UpdateFrequency = 10.0f;

    /** Enable debug logging */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Direct Widget Setup")
    bool bEnableDebugLogging = true;

protected:
    //----------------------------------------------------------
    // Internal Management - Handles everything automatically
    //----------------------------------------------------------

    /** Initialize UI subsystem */
    void InitializeUISubsystem();

    /** Update vehicle data periodically */
    UFUNCTION()
    void UpdateVehicleDataInternal();

    /** Setup update timer */
    void SetupUpdateTimer();

private:
    /** UI subsystem reference */
    UPROPERTY()
    TWeakObjectPtr<class URealGazeboUISubsystem> UISubsystem;

    /** Cached vehicle data */
    UPROPERTY()
    TArray<FUIVehicleData> CachedVehicleData;

    /** Update timer */
    FTimerHandle UpdateTimerHandle;

    /** Initialization flag */
    bool bIsInitialized;
};