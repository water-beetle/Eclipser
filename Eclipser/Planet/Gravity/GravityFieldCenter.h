// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GravityField.h"
#include "GravityFieldCenter.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ECLIPSER_API UGravityFieldCenter : public UGravityField
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UGravityFieldCenter();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;

public:
	virtual FVector GetGravityDirection(UGravityBody* GravityBody) const override;
};
