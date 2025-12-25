// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InputMappingContext.h"
#include "InputAction.h"
#include "Inv_PlayerController.generated.h"

class UInv_HudWidget;

UCLASS()
class INVENTORY_API AInv_PlayerController : public APlayerController
{
	GENERATED_BODY()

	AInv_PlayerController();
	
protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	virtual void Tick(float DeltaTime) override;
private:
	void PrimaryInteract();
	void CreateHudWidget();
	
	UPROPERTY(EditDefaultsOnly, Category = "Inventory")
	TArray<TObjectPtr<UInputMappingContext>> DefaultIMCs;

	UPROPERTY(EditDefaultsOnly, Category = "Inventory")
	TObjectPtr<UInputAction> PrimaryInterAction;

	UPROPERTY(EditDefaultsOnly, Category = "Inventory")
	TSubclassOf<UInv_HudWidget> HudWidgetClass;

	UPROPERTY()
	TObjectPtr<UInv_HudWidget> HudWidget;

private:
	void TraceForItem();
	void UpdatePickupMessagePosition();
	
	UPROPERTY(EditDefaultsOnly, Category = "Inventory")
	TEnumAsByte<ECollisionChannel> InteractionTraceChannel;
	
	float MaxItemTraceDistance = 300.f;

	TWeakObjectPtr<AActor> PreviousItem;
	TWeakObjectPtr<AActor> CurrentItem;
	
};
