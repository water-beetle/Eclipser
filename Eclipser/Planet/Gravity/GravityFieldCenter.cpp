// Fill out your copyright notice in the Description page of Project Settings.

#include "GravityFieldCenter.h"
#include "Character/GravityBody.h"

// Sets default values for this component's properties
UGravityFieldCenter::UGravityFieldCenter()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
	GravityScale = .5f;
	// ...
}


// Called when the game starts
void UGravityFieldCenter::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}


// Called every frame
void UGravityFieldCenter::TickComponent(float DeltaTime, ELevelTick TickType,
                                        FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

FVector UGravityFieldCenter::GetGravityDirection(UGravityBody* GravityBody) const
{
	return (GetOwner()->GetActorLocation() - GravityBody->GetOwner()->GetActorLocation()).GetSafeNormal();
}

