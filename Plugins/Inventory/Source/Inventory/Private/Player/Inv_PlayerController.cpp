#include "Player/Inv_PlayerController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "Inventory.h"
#include "Widgets/Inv_HudWidget.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"

void AInv_PlayerController::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogInventory, Log, TEXT("BeginPlay for PlayerContorller"));
	UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer());
	if (IsValid(Subsystem))
	{
		for (UInputMappingContext* CurrentContext : DefaultIMCs)
		{
			Subsystem->AddMappingContext(CurrentContext, 0);
		}
	}

	CreateHudWidget();
}

void AInv_PlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(InputComponent);
	EnhancedInputComponent->BindAction(PrimaryInterAction, ETriggerEvent::Started, this, &AInv_PlayerController::PrimaryInteract);
}

void AInv_PlayerController::PrimaryInteract()
{
	UE_LOG(LogTemp, Log, TEXT("Primary Interact"));
}

void AInv_PlayerController::CreateHudWidget()
{
	if (!IsLocalController()) return;

	HudWidget = CreateWidget<UInv_HudWidget>(this, HudWidgetClass);
	if (IsValid(HudWidget))
	{
		HudWidget->AddToViewport();
	}
}

void AInv_PlayerController::TraceForItem()
{
	if (!IsValid(GEngine) || !IsValid(GEngine->GameViewport)) return;
	
	FVector2D ViewportSize;
	GEngine->GameViewport->GetViewportSize(ViewportSize);
	const FVector2D ViewportCenter = ViewportSize / 2;
	
	FVector TraceStart;
	FVector Forward;
	if (!UGameplayStatics::DeprojectScreenToWorld(this, ViewportCenter, TraceStart, Forward)) return;

	const FVector TraceEnd = TraceStart + Forward * MaxItemTraceDistance;
	FHitResult Hit;

	bool bHit = GetWorld()->LineTraceSingleByChannel(Hit,TraceStart,TraceEnd,ItemTraceChannel);

	PreviousItem = CurrentItem;
	CurrentItem = Hit.GetActor();

	if (PreviousItem == CurrentItem) return;
	
}
