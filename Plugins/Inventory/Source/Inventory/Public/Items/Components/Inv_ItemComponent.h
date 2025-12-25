// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Inv_ItemComponent.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Blueprintable)
class INVENTORY_API UInv_ItemComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UInv_ItemComponent();

	FString GetPickupMessage() const {return PickupMessage;}

private:
	UPROPERTY(EditAnywhere,Category="Inventory")
	FString PickupMessage;
};
