#include "RealGazeboGameMode.h"
#include "RealGazeboPlayerController.h"
#include "Engine/Engine.h"
#include "Blueprint/UserWidget.h"
#include "UObject/ConstructorHelpers.h"

ARealGazeboGameMode::ARealGazeboGameMode()
{
    // Set the player controller class to our custom one
    PlayerControllerClass = ARealGazeboPlayerController::StaticClass();
    
    // No default pawn - we'll handle cameras manually
    DefaultPawnClass = nullptr;
    
    // Disable auto-possess
    bStartPlayersAsSpectators = false;

    // Initialize widget
    RealGazeboWidget = nullptr;
    
    // Try to find and set the default widget class (optional - can be set in Blueprint)
    static ConstructorHelpers::FClassFinder<UUserWidget> WidgetClassFinder(TEXT("/RealGazebo/UI/WBP_RealGazebo.WBP_RealGazebo_C"));
    if (WidgetClassFinder.Succeeded())
    {
        RealGazeboWidgetClass = WidgetClassFinder.Class;
        UE_LOG(LogTemp, Log, TEXT("RealGazeboGameMode: Found WBP_RealGazebo widget class"));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("RealGazeboGameMode: WBP_RealGazebo widget class not found - will need to be set manually"));
    }
}

void ARealGazeboGameMode::BeginPlay()
{
    Super::BeginPlay();
    
    // Create and show the RealGazebo widget
    CreateRealGazeboWidget();
    
    UE_LOG(LogTemp, Warning, TEXT("RealGazeboGameMode: Game started without default pawn"));
}

void ARealGazeboGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
    Super::InitGame(MapName, Options, ErrorMessage);
    
    UE_LOG(LogTemp, Warning, TEXT("RealGazeboGameMode: Initializing game for map: %s"), *MapName);
}

void ARealGazeboGameMode::CreateRealGazeboWidget()
{
    // Check if we have a valid widget class
    if (!RealGazeboWidgetClass)
    {
        UE_LOG(LogTemp, Error, TEXT("RealGazeboGameMode: RealGazeboWidgetClass is not set! Please set it in the GameMode settings or Blueprint."));
        return;
    }

    // Check if widget already exists
    if (RealGazeboWidget)
    {
        UE_LOG(LogTemp, Warning, TEXT("RealGazeboGameMode: RealGazebo widget already exists"));
        return;
    }

    // Get the player controller
    APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
    if (!PlayerController)
    {
        UE_LOG(LogTemp, Error, TEXT("RealGazeboGameMode: No player controller found"));
        return;
    }

    // Create the widget
    RealGazeboWidget = CreateWidget<UUserWidget>(PlayerController, RealGazeboWidgetClass);
    if (RealGazeboWidget)
    {
        // Add to viewport
        RealGazeboWidget->AddToViewport();
        
        // Set input mode to allow both game and UI input
        FInputModeGameAndUI InputMode;
        InputMode.SetWidgetToFocus(nullptr); // Don't focus the widget initially
        InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::LockOnCapture);
        InputMode.SetHideCursorDuringCapture(true);
        PlayerController->SetInputMode(InputMode);
        
        // Show mouse cursor for UI interaction
        PlayerController->bShowMouseCursor = true;
        PlayerController->bEnableClickEvents = true;
        PlayerController->bEnableMouseOverEvents = true;
        
        UE_LOG(LogTemp, Warning, TEXT("RealGazeboGameMode: RealGazebo widget created and added to viewport"));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("RealGazeboGameMode: Failed to create RealGazebo widget"));
    }
}

void ARealGazeboGameMode::ShowRealGazeboWidget()
{
    if (RealGazeboWidget)
    {
        RealGazeboWidget->SetVisibility(ESlateVisibility::Visible);
        UE_LOG(LogTemp, Log, TEXT("RealGazeboGameMode: RealGazebo widget shown"));
    }
}

void ARealGazeboGameMode::HideRealGazeboWidget()
{
    if (RealGazeboWidget)
    {
        RealGazeboWidget->SetVisibility(ESlateVisibility::Hidden);
        UE_LOG(LogTemp, Log, TEXT("RealGazeboGameMode: RealGazebo widget hidden"));
    }
}

void ARealGazeboGameMode::ToggleRealGazeboWidget()
{
    if (RealGazeboWidget)
    {
        ESlateVisibility CurrentVisibility = RealGazeboWidget->GetVisibility();
        if (CurrentVisibility == ESlateVisibility::Visible)
        {
            HideRealGazeboWidget();
        }
        else
        {
            ShowRealGazeboWidget();
        }
    }
}

bool ARealGazeboGameMode::IsRealGazeboWidgetVisible() const
{
    if (RealGazeboWidget)
    {
        return RealGazeboWidget->GetVisibility() == ESlateVisibility::Visible;
    }
    return false;
}