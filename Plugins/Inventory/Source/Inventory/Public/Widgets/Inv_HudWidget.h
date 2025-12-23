// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Inv_HudWidget.generated.h"

/**
 * 
 */
UCLASS()
class INVENTORY_API UInv_HudWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintImplementableEvent, Category = "Inventory")
	void ShowPickupMessage(const FString& Message);

	UFUNCTION(BlueprintImplementableEvent, Category = "Inventory")
	void HidePickupMessage(const FString& Message);
};
