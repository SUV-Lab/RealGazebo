// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Vehicle/GazeboVehicleData.h"
#include "GazeboVehicleInfoWidget.generated.h"

class UTextBlock;
class AGazeboVehicleActor;

/**
 * Widget to display vehicle information (ID, Position, Status)
 */
UCLASS(BlueprintType, Blueprintable)
class REALGAZEBOUI_API UGazeboVehicleInfoWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UGazeboVehicleInfoWidget(const FObjectInitializer& ObjectInitializer);

protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

public:
    // Widget management
    UFUNCTION(BlueprintCallable, Category = "RealGazebo|UI")
    void SetTargetVehicle(AGazeboVehicleActor* Vehicle);

    UFUNCTION(BlueprintCallable, Category = "RealGazebo|UI")
    void ClearTargetVehicle();

    UFUNCTION(BlueprintCallable, Category = "RealGazebo|UI")
    void UpdateVehicleInfo();

    // Manual data updates (for testing)
    UFUNCTION(BlueprintCallable, Category = "RealGazebo|UI")
    void UpdateVehicleInfoManual(int32 VehicleID, const FVector& Position, const FRotator& Rotation);

protected:
    // Widget references (bind in Blueprint)
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "RealGazebo|UI")
    UTextBlock* VehicleIDText;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "RealGazebo|UI")
    UTextBlock* VehicleTypeText;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "RealGazebo|UI")
    UTextBlock* PositionXText;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "RealGazebo|UI")
    UTextBlock* PositionYText;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "RealGazebo|UI")
    UTextBlock* PositionZText;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "RealGazebo|UI")
    UTextBlock* RotationPitchText;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "RealGazebo|UI")
    UTextBlock* RotationYawText;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "RealGazebo|UI")
    UTextBlock* RotationRollText;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "RealGazebo|UI")
    UTextBlock* StatusText;

    // Configuration
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealGazebo|UI Settings")
    float UpdateFrequency = 10.0f; // Updates per second

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealGazebo|UI Settings")
    int32 DecimalPlaces = 2;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealGazebo|UI Settings")
    bool bShowDebugInfo = false;

private:
    UPROPERTY()
    AGazeboVehicleActor* TargetVehicle = nullptr;

    float UpdateTimer = 0.0f;
    FGazeboPoseData LastPoseData;
    
    // Event handlers
    UFUNCTION()
    void OnVehiclePoseReceived(const FGazeboPoseData& PoseData);

    // UI update helpers
    void UpdateVehicleIDDisplay(int32 VehicleID, uint8 VehicleType);
    void UpdatePositionDisplay(const FVector& Position);
    void UpdateRotationDisplay(const FRotator& Rotation);
    void UpdateStatusDisplay(bool bIsReceivingData);

    FString FormatFloat(float Value, int32 Decimals = 2) const;
    FString GetVehicleTypeName(uint8 VehicleType) const;
};