// Fill out your copyright notice in the Description page of Project Settings.


#include "Planet.h"

#include "Gravity/GravityFieldCenter.h"
#include "Voxel/VoxelManager.h"


// Sets default values
APlanet::APlanet()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	VoxelManager = CreateDefaultSubobject<UVoxelManager>("VoxelManager");
	GravityField = CreateDefaultSubobject<UGravityFieldCenter>("GravityField");
	GravityField->SetupAttachment(RootComponent);
}

// Called when the game starts or when spawned
void APlanet::BeginPlay()
{
	Super::BeginPlay();

	PlanetRadius = VoxelManager->GetPlanetRadius();
	GravityField->SetGravityFieldSize(PlanetRadius);
}

// Called every frame
void APlanet::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

