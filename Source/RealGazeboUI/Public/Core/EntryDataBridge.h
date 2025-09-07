#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Core/UIVehicleDataInterface.h"
#include "EntryDataBridge.generated.h"

/**
 * Simple data structure for CtrackEntry integration
 * Converts RealGazebo vehicle data to format suitable for existing UI
 */
USTRUCT(BlueprintType)
struct REALGAZEBOUI_API FCtrackEntryData
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, Category = "Entry Data")
    FString VehicleName;

    UPROPERTY(BlueprintReadWrite, Category = "Entry Data")
    int32 VehicleID;

    UPROPERTY(BlueprintReadWrite, Category = "Entry Data")
    FString VehicleType;

    UPROPERTY(BlueprintReadWrite, Category = "Entry Data")
    FVector Position;

    UPROPERTY(BlueprintReadWrite, Category = "Entry Data")
    FRotator Rotation;

    UPROPERTY(BlueprintReadWrite, Category = "Entry Data")
    bool bIsActive;

    UPROPERTY(BlueprintReadWrite, Category = "Entry Data")
    FString StatusText;

    FCtrackEntryData()
    {
        VehicleName = TEXT("Unknown");
        VehicleID = 0;
        VehicleType = TEXT("Generic");
        Position = FVector::ZeroVector;
        Rotation = FRotator::ZeroRotator;
        bIsActive = false;
        StatusText = TEXT("Inactive");
    }
};

/**
 * EntryDataBridge - Converts RealGazebo vehicle data to CtrackEntry format
 * Simple, focused class for data conversion and integration
 */
UCLASS(BlueprintType, Blueprintable)
class REALGAZEBOUI_API UEntryDataBridge : public UObject
{
    GENERATED_BODY()

public:
    UEntryDataBridge();

    /** Convert RealGazebo vehicle data to CtrackEntry format */
    UFUNCTION(BlueprintCallable, Category = "Entry Data Bridge")
    FCtrackEntryData ConvertVehicleData(const FUIVehicleData& VehicleData);

    /** Convert array of RealGazebo vehicle data to CtrackEntry format */
    UFUNCTION(BlueprintCallable, Category = "Entry Data Bridge")
    TArray<FCtrackEntryData> ConvertVehicleDataArray(const TArray<FUIVehicleData>& VehicleDataArray);

    /** Get single vehicle data as CtrackEntry format */
    UFUNCTION(BlueprintCallable, Category = "Entry Data Bridge")
    FCtrackEntryData GetVehicleAsCtrackEntry(const FString& VehicleName);

    /** Get all vehicles as CtrackEntry format */
    UFUNCTION(BlueprintCallable, Category = "Entry Data Bridge")
    TArray<FCtrackEntryData> GetAllVehiclesAsCtrackEntries();

    /** Check if vehicle exists */
    UFUNCTION(BlueprintCallable, Category = "Entry Data Bridge")
    bool IsVehicleActive(const FString& VehicleName);

    /** Get vehicle count */
    UFUNCTION(BlueprintCallable, Category = "Entry Data Bridge")
    int32 GetActiveVehicleCount();

private:
    /** Cached UI subsystem reference */
    UPROPERTY()
    TWeakObjectPtr<class URealGazeboUISubsystem> UISubsystem;

    /** Initialize UI subsystem connection */
    void InitializeUISubsystem();

    /** Helper to generate status text */
    FString GenerateStatusText(const FUIVehicleData& VehicleData);
};