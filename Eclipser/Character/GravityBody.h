// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GravityBody.generated.h"

class UGravityField;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ECLIPSER_API UGravityBody : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UGravityBody();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
							   FActorComponentTickFunction* ThisTickFunction) override;
public:
	void AddGravityArea(UGravityField* GravityField);
	void RemoveGravityArea(UGravityField* GravityField);
	FVector GetGravityDirection();

private:
	TArray<UGravityField*> GravityFields;
	UGravityField* CurrentGravityField;
};
