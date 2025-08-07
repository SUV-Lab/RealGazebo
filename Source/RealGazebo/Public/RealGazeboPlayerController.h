#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "UserCameraManager.h" 
#include "RealGazeboPlayerController.generated.h"

UCLASS()
class REALGAZEBO_API ARealGazeboPlayerController : public APlayerController
{
    GENERATED_BODY()

public:
    ARealGazeboPlayerController();

protected:
    virtual void BeginPlay() override;
    virtual void SetupInputComponent() override;
    virtual void Tick(float DeltaTime) override;

private:
    // Input handlers
    void OnManualCameraPressed();
    void OnFirstPersonCameraPressed();
    void OnThirdPersonCameraPressed();
    //void OnNextVehiclePressed();
    //void OnPreviousVehiclePressed();
    void OnToggleUIPressed();

public:
    // UI-based vehicle selection
    UFUNCTION(BlueprintCallable, Category = "RealGazebo|UI")
    void SelectVehicleByIndex(int32 VehicleIndex);
    
    UFUNCTION(BlueprintPure, Category = "RealGazebo|UI")
    TArray<FString> GetAvailableVehicleNames() const;
    
    UFUNCTION(BlueprintPure, Category = "RealGazebo|UI")
    int32 GetCurrentVehicleIndex() const;
    
    // UI helper functions for displaying current state
    UFUNCTION(BlueprintPure, Category = "RealGazebo|UI")
    FString GetCurrentCameraModeString() const;
    
    UFUNCTION(BlueprintPure, Category = "RealGazebo|UI")
    FString GetCurrentVehicleName() const;

private:
    // Camera manager reference - updated type
    UPROPERTY()
    UUserCameraManager* CameraManager;

    // Find camera manager in the scene
    void FindCameraManager();

    // UI toggle state
    bool bShowUI;
};