// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/GazeboVehicleInfoWidget.h"
#include "Vehicle/GazeboVehicleActor.h"
#include "Components/TextBlock.h"

UGazeboVehicleInfoWidget::UGazeboVehicleInfoWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    // Enable ticking for real-time updates
    bHasScriptImplementedTick = true;
}

void UGazeboVehicleInfoWidget::NativeConstruct()
{
    Super::NativeConstruct();
    
    // Initialize display with default values
    UpdateStatusDisplay(false);
    UE_LOG(LogTemp, Log, TEXT("GazeboVehicleInfoWidget: Constructed"));
}

void UGazeboVehicleInfoWidget::NativeDestruct()
{
    ClearTargetVehicle();
    Super::NativeDestruct();
}

void UGazeboVehicleInfoWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    if (!TargetVehicle) return;

    UpdateTimer += InDeltaTime;
    if (UpdateTimer >= (1.0f / UpdateFrequency))
    {
        UpdateVehicleInfo();
        UpdateTimer = 0.0f;
    }
}

void UGazeboVehicleInfoWidget::SetTargetVehicle(AGazeboVehicleActor* Vehicle)
{
    // Clear previous vehicle binding
    ClearTargetVehicle();

    TargetVehicle = Vehicle;
    if (TargetVehicle)
    {
        // TODO: Bind to global data receiver or vehicle manager's events
        // For now, we'll update from Tick
        UpdateVehicleInfo();
        UE_LOG(LogTemp, Log, TEXT("GazeboVehicleInfoWidget: Target vehicle set"));
    }
}

void UGazeboVehicleInfoWidget::ClearTargetVehicle()
{
    if (TargetVehicle)
    {
        // TODO: Unbind from global data receiver events when implemented
        TargetVehicle = nullptr;
    }
}

void UGazeboVehicleInfoWidget::UpdateVehicleInfo()
{
    if (!TargetVehicle) return;

    // Get current vehicle data
    FVector CurrentPosition = TargetVehicle->GetActorLocation();
    FRotator CurrentRotation = TargetVehicle->GetActorRotation();
    
    // Update displays
    UpdatePositionDisplay(CurrentPosition);
    UpdateRotationDisplay(CurrentRotation);
    UpdateStatusDisplay(true); // Vehicle is active if we're updating
}

void UGazeboVehicleInfoWidget::UpdateVehicleInfoManual(int32 VehicleID, const FVector& Position, const FRotator& Rotation)
{
    UpdateVehicleIDDisplay(VehicleID, 0); // Default vehicle type
    UpdatePositionDisplay(Position);
    UpdateRotationDisplay(Rotation);
    UpdateStatusDisplay(true);
}

void UGazeboVehicleInfoWidget::OnVehiclePoseReceived(const FGazeboPoseData& PoseData)
{
    LastPoseData = PoseData;
    
    // Update ID and type info
    UpdateVehicleIDDisplay(PoseData.VehicleNum, PoseData.VehicleType);
    
    // Position and rotation are already updated in NativeTick
    // This ensures smooth interpolation
}

void UGazeboVehicleInfoWidget::UpdateVehicleIDDisplay(int32 VehicleID, uint8 VehicleType)
{
    if (VehicleIDText)
    {
        VehicleIDText->SetText(FText::FromString(FString::Printf(TEXT("%d"), VehicleID)));
    }

    if (VehicleTypeText)
    {
        FString TypeName = GetVehicleTypeName(VehicleType);
        VehicleTypeText->SetText(FText::FromString(TypeName));
    }
}

void UGazeboVehicleInfoWidget::UpdatePositionDisplay(const FVector& Position)
{
    if (PositionXText)
    {
        PositionXText->SetText(FText::FromString(FormatFloat(Position.X, DecimalPlaces)));
    }
    if (PositionYText)
    {
        PositionYText->SetText(FText::FromString(FormatFloat(Position.Y, DecimalPlaces)));
    }
    if (PositionZText)
    {
        PositionZText->SetText(FText::FromString(FormatFloat(Position.Z, DecimalPlaces)));
    }
}

void UGazeboVehicleInfoWidget::UpdateRotationDisplay(const FRotator& Rotation)
{
    if (RotationPitchText)
    {
        RotationPitchText->SetText(FText::FromString(FormatFloat(Rotation.Pitch, DecimalPlaces) + TEXT("°")));
    }
    if (RotationYawText)
    {
        RotationYawText->SetText(FText::FromString(FormatFloat(Rotation.Yaw, DecimalPlaces) + TEXT("°")));
    }
    if (RotationRollText)
    {
        RotationRollText->SetText(FText::FromString(FormatFloat(Rotation.Roll, DecimalPlaces) + TEXT("°")));
    }
}

void UGazeboVehicleInfoWidget::UpdateStatusDisplay(bool bIsReceivingData)
{
    if (StatusText)
    {
        FString StatusString = bIsReceivingData ? TEXT("ACTIVE") : TEXT("INACTIVE");
        FLinearColor StatusColor = bIsReceivingData ? FLinearColor::Green : FLinearColor::Red;
        
        StatusText->SetText(FText::FromString(StatusString));
        StatusText->SetColorAndOpacity(FSlateColor(StatusColor));
    }
}

FString UGazeboVehicleInfoWidget::FormatFloat(float Value, int32 Decimals) const
{
    switch (Decimals)
    {
    case 0: return FString::Printf(TEXT("%.0f"), Value);
    case 1: return FString::Printf(TEXT("%.1f"), Value);
    case 2: return FString::Printf(TEXT("%.2f"), Value);
    case 3: return FString::Printf(TEXT("%.3f"), Value);
    default: return FString::Printf(TEXT("%.2f"), Value);
    }
}

FString UGazeboVehicleInfoWidget::GetVehicleTypeName(uint8 VehicleType) const
{
    // This should match the GazeboVehicleManager's vehicle types
    switch (VehicleType)
    {
    case 0: return TEXT("Iris");
    case 1: return TEXT("Rover");
    case 2: return TEXT("Plane");
    case 3: return TEXT("Copter");
    default: return TEXT("Unknown");
    }
}