// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/Inventory/InventoryBase/Inv_InventoryBase.h"
#include "Inv_SpatialInventory.generated.h"

class UButton;
class UWidgetSwitcher;
class UInv_InventoryGrid;
/**
 * 
 */
UCLASS()
class INVENTORY_API UInv_SpatialInventory : public UInv_InventoryBase
{
	GENERATED_BODY()
public:
	virtual void NativeOnInitialized() override;
	
private:
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UWidgetSwitcher> Switcher;
	
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UInv_InventoryGrid> Grid_Equippable;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UInv_InventoryGrid> Grid_Consumable;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UInv_InventoryGrid> Grid_Craftable;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UButton> Button_Equippable;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UButton> Button_Consumable;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UButton> Button_Craftable;

	UFUNCTION()
	void ShowEquippable();
	UFUNCTION()
	void ShowConsumable();
	UFUNCTION()
	void ShowCraftable();
	void DisableButton(UButton* Button);
	void SetActiveGrid(UInv_InventoryGrid* Grid, UButton* Button);
};
