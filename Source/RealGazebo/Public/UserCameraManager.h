#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MainFreeCameraActor.h"
#include "GazeboVehicleActor.h"
#include "UserCameraManager.generated.h"

UENUM(BlueprintType)
enum class EUserCameraMode : uint8
{
    MainFree UMETA(DisplayName = "Main Free Camera"),
    VehicleFirstPerson UMETA(DisplayName = "Vehicle First Person"),
    VehicleThirdPerson UMETA(DisplayName = "Vehicle Third Person")
};

UCLASS(ClassGroup=(RealGazebo), meta=(BlueprintSpawnableComponent))
class REALGAZEBO_API UUserCameraManager : public UActorComponent
{
    GENERATED_BODY()

public:
    UUserCameraManager();

protected:
    virtual void BeginPlay() override;

public:
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    // Main Free Camera Settings
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealGazebo|User Camera Manager")
    TSubclassOf<AMainFreeCameraActor> MainFreeCameraClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealGazebo|User Camera Manager")
    FVector MainCameraSpawnLocation = FVector(0.0f, 0.0f, 500.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealGazebo|User Camera Manager")
    FRotator MainCameraSpawnRotation = FRotator(0.0f, 0.0f, 0.0f);

    // Control Functions
    UFUNCTION(BlueprintCallable, Category = "RealGazebo|User Camera Manager")
    void SwitchToMainFreeCamera();

    UFUNCTION(BlueprintCallable, Category = "RealGazebo|User Camera Manager")
    void SwitchToVehicleFirstPerson();

    UFUNCTION(BlueprintCallable, Category = "RealGazebo|User Camera Manager")
    void SwitchToVehicleThirdPerson();

    UFUNCTION(BlueprintCallable, Category = "RealGazebo|User Camera Manager")
    void CycleToNextVehicle();

    UFUNCTION(BlueprintCallable, Category = "RealGazebo|User Camera Manager")
    void CycleToPreviousVehicle();

    UFUNCTION(BlueprintCallable, Category = "RealGazebo|User Camera Manager")
    void CycleCameraMode();

    // Getters
    UFUNCTION(BlueprintPure, Category = "RealGazebo|User Camera Manager")
    EUserCameraMode GetCurrentCameraMode() const { return CurrentCameraMode; }

    UFUNCTION(BlueprintPure, Category = "RealGazebo|User Camera Manager")
    AGazeboVehicleActor* GetSelectedVehicle() const;

    UFUNCTION(BlueprintPure, Category = "RealGazebo|User Camera Manager")
    TArray<AGazeboVehicleActor*> GetAvailableVehicles() const;

    UFUNCTION(BlueprintPure, Category = "RealGazebo|User Camera Manager")
    FString GetVehicleDisplayName(AGazeboVehicleActor* Vehicle) const;

    UFUNCTION(BlueprintPure, Category = "RealGazebo|User Camera Manager")
    AMainFreeCameraActor* GetMainFreeCamera() const { return MainFreeCamera; }

    UFUNCTION(BlueprintPure, Category = "RealGazebo|User Camera Manager")
    int32 GetSelectedVehicleIndex() const { return SelectedVehicleIndex; }

    UFUNCTION(BlueprintCallable, Category = "RealGazebo|User Camera Manager")
    void SetSelectedVehicleIndex(int32 NewIndex);

    // Vehicle spawn event handler
    UFUNCTION(BlueprintCallable, Category = "RealGazebo|User Camera Manager")
    void OnVehicleSpawned(AGazeboVehicleActor* NewVehicle);

    // New function for smooth MainFreeCamera positioning
    UFUNCTION(BlueprintCallable, Category = "RealGazebo|User Camera Manager")
    void MoveMainFreeCameraToVehiclePosition();

    // Configuration for smooth transitions
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealGazebo|User Camera Manager")
    float CameraTransitionSpeed = 5.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealGazebo|User Camera Manager")
    bool bUseSmoothMainCameraTransition = true;

private:
    // Camera state
    EUserCameraMode CurrentCameraMode;
    int32 SelectedVehicleIndex;

    // Main free camera reference
    UPROPERTY()
    AMainFreeCameraActor* MainFreeCamera;

    // Track last vehicle camera state for M key functionality
    FVector LastVehicleCameraLocation;
    FRotator LastVehicleCameraRotation;
    EUserCameraMode LastVehicleCameraMode;
    bool bHasValidLastVehicleCamera;

    // Smooth transition state
    bool bIsTransitioningMainCamera;
    FVector MainCameraTargetLocation;
    FRotator MainCameraTargetRotation;

    // Internal functions
    void SpawnMainFreeCamera();
    void DeactivateAllCameras();
    void InitializeVehicleSelection();
    void UpdateLastVehicleCameraState();
    void StartMainCameraTransition(const FVector& TargetLocation, const FRotator& TargetRotation);
    void UpdateMainCameraTransition(float DeltaTime);
    FVector GetVehicleCameraLocation() const;
    FRotator GetVehicleCameraRotation() const;
    void UpdateMainFreeCameraToCurrentView();
};