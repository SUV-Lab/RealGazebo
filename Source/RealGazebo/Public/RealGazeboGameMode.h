#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Blueprint/UserWidget.h"
#include "RealGazeboGameMode.generated.h"

UCLASS()
class REALGAZEBO_API ARealGazeboGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    ARealGazeboGameMode();

protected:
    virtual void BeginPlay() override;
    virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;

    // Widget Configuration
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealGazebo|UI")
    TSubclassOf<UUserWidget> RealGazeboWidgetClass;

private:
    // Widget instance
    UPROPERTY()
    UUserWidget* RealGazeboWidget;

    // UI management functions
    void CreateRealGazeboWidget();
    void ShowRealGazeboWidget();
    void HideRealGazeboWidget();

public:
    // Public functions for toggling UI visibility
    UFUNCTION(BlueprintCallable, Category = "RealGazebo|UI")
    void ToggleRealGazeboWidget();

    UFUNCTION(BlueprintCallable, Category = "RealGazebo|UI")
    bool IsRealGazeboWidgetVisible() const;
};