#include "Player/Inv_PlayerController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "Inventory.h"
#include "Widgets/HUD/Inv_HudWidget.h"
#include "Blueprint/UserWidget.h"
#include "Interaction/Inv_HighlightableStaticMesh.h"
#include "InventoryManagement/Components/Inv_InventoryComponent.h"
#include "Items/Components/Inv_ItemComponent.h"
#include "Kismet/GameplayStatics.h"

AInv_PlayerController::AInv_PlayerController()
{
	PrimaryActorTick.bCanEverTick = true;
	MaxItemTraceDistance = 500.0f;
	InteractionTraceChannel = ECollisionChannel::ECC_GameTraceChannel1;
}

void AInv_PlayerController::ToggleInventory()
{
	if (!InventoryComponent.IsValid()) return;

	InventoryComponent->ToggleInventoryMenu();
}

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

	InventoryComponent = FindComponentByClass<UInv_InventoryComponent>();
	
	CreateHudWidget();
}

void AInv_PlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(InputComponent);
	EnhancedInputComponent->BindAction(PrimaryInterAction, ETriggerEvent::Started, this, &AInv_PlayerController::PrimaryInteract);
	EnhancedInputComponent->BindAction(ToggleInventoryAction, ETriggerEvent::Started, this, &AInv_PlayerController::ToggleInventory);
}

void AInv_PlayerController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	TraceForItem();
	UpdatePickupMessagePosition();
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

	bool bHit = GetWorld()->LineTraceSingleByChannel(Hit,TraceStart,TraceEnd,InteractionTraceChannel);

	PreviousItem = CurrentItem;
	CurrentItem  = bHit ? Hit.GetActor() : nullptr;

	if (!CurrentItem.IsValid())
	{
		if (IsValid(HudWidget)) HudWidget->HidePickupMessage();
	}
	
	if (PreviousItem == CurrentItem) return;

	if (CurrentItem.IsValid())
	{
		UActorComponent* Highlightable = CurrentItem->FindComponentByInterface(UInv_Highlightable::StaticClass());
		if (IsValid(Highlightable))
		{
			IInv_Highlightable::Execute_Highlight(Highlightable);
		}
		
		UInv_ItemComponent* ItemComponent = CurrentItem->FindComponentByClass<UInv_ItemComponent>();
		if (!IsValid(ItemComponent)) return;

		if (IsValid(HudWidget)) HudWidget->ShowPickupMessage(ItemComponent->GetPickupMessage());
	}

	if (PreviousItem.IsValid())
	{
		UActorComponent* Highlightable = PreviousItem->FindComponentByInterface(UInv_Highlightable::StaticClass());
		if (IsValid(Highlightable))
		{
			IInv_Highlightable::Execute_UnHighlight(Highlightable);
		}
	}
	
}

void AInv_PlayerController::UpdatePickupMessagePosition()
{
	if (!CurrentItem.IsValid() || !IsValid(HudWidget)) return;

	FVector WorldPos = CurrentItem->GetActorLocation() + FVector(0.f, 0.f, 60.f);

	FVector2D ScreenPos;
	const bool bOnScreen = ProjectWorldLocationToScreen(WorldPos, ScreenPos, true);

	if (!bOnScreen)
	{
		HudWidget->HidePickupMessage();
		return;
	}
	ScreenPos += FVector2D(20.f, -10.f);
	HudWidget->SetPickupMessageScreenPosition(ScreenPos);
}
