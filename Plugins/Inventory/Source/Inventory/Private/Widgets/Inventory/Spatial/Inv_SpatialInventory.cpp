// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/Inventory/Spatial/Inv_SpatialInventory.h"
#include "Widgets/Inventory/Spatial/Inv_InventoryGrid.h"
#include "Components/Button.h"
#include "Components/WidgetSwitcher.h"

void UInv_SpatialInventory::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	Button_Equippable->OnClicked.AddDynamic(this, &ThisClass::UInv_SpatialInventory::ShowEquippable);
	Button_Consumable->OnClicked.AddDynamic(this, &ThisClass::UInv_SpatialInventory::ShowConsumable);
	Button_Craftable->OnClicked.AddDynamic(this, &ThisClass::UInv_SpatialInventory::ShowCraftable);

	ShowEquippable();
}

void UInv_SpatialInventory::ShowEquippable()
{
	SetActiveGrid(Grid_Equippable, Button_Equippable);
}

void UInv_SpatialInventory::ShowConsumable()
{
	SetActiveGrid(Grid_Consumable, Button_Consumable);
}

void UInv_SpatialInventory::ShowCraftable()
{
	SetActiveGrid(Grid_Craftable, Button_Craftable);
}

void UInv_SpatialInventory::DisableButton(UButton* Button)
{
	Button_Equippable->SetIsEnabled(true);
	Button_Consumable->SetIsEnabled(true);
	Button_Craftable->SetIsEnabled(true);
	Button->SetIsEnabled(false);
}

void UInv_SpatialInventory::SetActiveGrid(UInv_InventoryGrid* Grid, UButton* Button)
{
	DisableButton(Button);
	Switcher->SetActiveWidget(Grid);
}
