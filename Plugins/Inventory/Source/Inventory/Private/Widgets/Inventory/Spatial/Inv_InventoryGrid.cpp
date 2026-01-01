// Fill out your copyright notice in the Description page of Project Settings.
#include "Widgets/Inventory/Spatial/Inv_InventoryGrid.h"
#include "Components/CanvasPanel.h"
#include "Widgets/Inventory/GridSlots/Inv_GridSlot.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/CanvasPanelSlot.h"
#include "Widgets/Utils/Inv_WidgetUtils.h"

void UInv_InventoryGrid::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	ConstructGrid();
}

void UInv_InventoryGrid::ConstructGrid()
{
	GridSlots.Reserve(Rows * Cols);

	for (int32 j = 0; j < Rows; j++)
	{
		for (int32 i = 0; i < Cols; i++)
		{
			UInv_GridSlot* GridSlot = CreateWidget<UInv_GridSlot>(this, GridSlotClass);
			CanvasPanel->AddChild(GridSlot);

			const FIntPoint TilePositoin(i, j);
			GridSlot->SetTileIndex(UInv_WidgetUtils::GetIndexFromPosition(TilePositoin, Cols));

			UCanvasPanelSlot* GridCPS = UWidgetLayoutLibrary::SlotAsCanvasSlot(GridSlot);
			GridCPS->SetSize(FVector2D(TileSize));
			GridCPS->SetPosition(TilePositoin * TileSize);			

			GridSlots.Add(GridSlot);
		}
	}
}
