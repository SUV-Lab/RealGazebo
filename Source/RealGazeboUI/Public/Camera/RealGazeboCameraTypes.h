#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "RealGazeboCameraTypes.generated.h"

UENUM(BlueprintType)
enum class ERealGazeboCameraMode : uint8
{
    /** Free-flying camera with WASD+mouse controls */
    Manual        = 0   UMETA(DisplayName = "Manual Camera"),
    
    /** Fixed camera at vehicle center (cockpit view) */
    FirstPerson   = 1   UMETA(DisplayName = "First Person Camera"),
    
    /** Spring arm following camera behind vehicle */
    ThirdPerson   = 2   UMETA(DisplayName = "Third Person Camera"),
    
    /** No camera active */
    None          = 3   UMETA(DisplayName = "No Camera")
};

UENUM(BlueprintType)
enum class ECameraTransitionType : uint8
{
    /** Instant camera switch */
    Instant       = 0   UMETA(DisplayName = "Instant"),
    
    /** Smooth interpolated transition */
    Smooth        = 1   UMETA(DisplayName = "Smooth"),
    
    /** Cinematic transition with timeline */
    Cinematic     = 2   UMETA(DisplayName = "Cinematic")
};

USTRUCT(BlueprintType)
struct REALGAZEBOUI_API FCameraSettings
{
    GENERATED_BODY()

    /** Camera field of view */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = "5.0", ClampMax = "170.0"))
    float FieldOfView = 90.0f;

    /** Spring arm length for third person camera */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = "50.0", ClampMax = "2000.0"))
    float SpringArmLength = 400.0f;

    /** Camera transition speed */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = "1.0", ClampMax = "20.0"))
    float TransitionSpeed = 5.0f;

    /** Camera transition type */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
    ECameraTransitionType TransitionType = ECameraTransitionType::Smooth;

    /** Enable camera lag for spring arm */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
    bool bEnableCameraLag = true;

    /** Camera lag speed */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = "1.0", ClampMax = "25.0", EditCondition = "bEnableCameraLag"))
    float CameraLagSpeed = 10.0f;

    FCameraSettings()
    {
        FieldOfView = 90.0f;
        SpringArmLength = 400.0f;
        TransitionSpeed = 5.0f;
        TransitionType = ECameraTransitionType::Smooth;
        bEnableCameraLag = true;
        CameraLagSpeed = 10.0f;
    }
};

USTRUCT(BlueprintType)
struct REALGAZEBOUI_API FManualCameraSettings
{
    GENERATED_BODY()

    /** Movement speed for manual camera */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Manual Camera", meta = (ClampMin = "100.0", ClampMax = "5000.0"))
    float MovementSpeed = 1000.0f;

    /** Mouse sensitivity for look */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Manual Camera", meta = (ClampMin = "0.1", ClampMax = "10.0"))
    float MouseSensitivity = 2.0f;

    /** Enable acceleration for movement */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Manual Camera")
    bool bEnableAcceleration = true;

    /** Acceleration multiplier */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Manual Camera", meta = (ClampMin = "1.0", ClampMax = "10.0", EditCondition = "bEnableAcceleration"))
    float AccelerationMultiplier = 2.0f;

    FManualCameraSettings()
    {
        MovementSpeed = 1000.0f;
        MouseSensitivity = 2.0f;
        bEnableAcceleration = true;
        AccelerationMultiplier = 2.0f;
    }
};