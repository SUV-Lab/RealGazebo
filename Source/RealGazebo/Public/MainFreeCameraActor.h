#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Camera/CameraComponent.h"
#include "Components/InputComponent.h"
#include "MainFreeCameraActor.generated.h"

UCLASS()
class REALGAZEBO_API AMainFreeCameraActor : public APawn
{
    GENERATED_BODY()

public:
    AMainFreeCameraActor();

protected:
    virtual void BeginPlay() override;

public:
    virtual void Tick(float DeltaTime) override;
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

    // Components
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RealGazebo|Main Camera")
    USceneComponent* RootSceneComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RealGazebo|Main Camera")
    UCameraComponent* MainCamera;

    // Camera Settings
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealGazebo|Main Camera Settings")
    float CameraSpeed = 1000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealGazebo|Main Camera Settings")
    float MouseSensitivity = 2.0f;

    // Control Functions
    UFUNCTION(BlueprintCallable, Category = "RealGazebo|Main Camera")
    void ActivateMainCamera();

    UFUNCTION(BlueprintCallable, Category = "RealGazebo|Main Camera")
    void DeactivateMainCamera();

    UFUNCTION(BlueprintPure, Category = "RealGazebo|Main Camera")
    bool IsMainCameraActive() const;

protected:
    // Input handlers
    
    void MoveForward(float Value);
    void MoveRight(float Value);
    void MoveUp(float Value);
    void LookUp(float Value);
    void LookRight(float Value);

private:
    FVector2D MouseInput;
    bool bIsActive;
};
