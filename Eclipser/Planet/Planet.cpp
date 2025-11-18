// Fill out your copyright notice in the Description page of Project Settings.


#include "Planet.h"

#include "Gravity/GravityFieldCenter.h"
#include "Voxel/VoxelManager.h"


// Sets default values
APlanet::APlanet()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	
	PlanetRoot = CreateDefaultSubobject<USceneComponent>(TEXT("PlanetRoot"));
	SetRootComponent(PlanetRoot);
	
	VoxelManager = CreateDefaultSubobject<UVoxelManager>("VoxelManager");
	VoxelManager->SetupAttachment(PlanetRoot);
	GravityField = CreateDefaultSubobject<UGravityFieldCenter>("GravityField");
	GravityField->SetupAttachment(PlanetRoot);
}

// Called when the game starts or when spawned
void APlanet::BeginPlay()
{
	Super::BeginPlay();

	UpdatePlanetConfiguration();
}

void APlanet::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	UpdatePlanetConfiguration();
}

// Called every frame
void APlanet::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void APlanet::UpdatePlanetConfiguration()
{
	if (!VoxelManager)
	{
		PlanetDiameter = 0;
		PlanetRadius = 0;
		return;
	}

	if (!HasValidVoxelConfiguration())
	{
		CacheVoxelSettingsFromManager();
	}

	if (!HasValidVoxelConfiguration())
	{
		PlanetDiameter = 0;
		PlanetRadius = 0;
		return;
	}

	VoxelManager->SetVoxelSettings(CellSize, CellNum, ChunkNum);

	PlanetDiameter = CalculatePlanetDiameter();
	if (PlanetRadius < 0 || PlanetRadius > PlanetDiameter / 2)
		PlanetRadius = PlanetDiameter / 2;

	if (GravityField)
	{
		GravityField->SetGravityFieldSize(PlanetRadius * 1.5);
	}

	VoxelManager->SetPlanetRadius(PlanetRadius);
}

bool APlanet::HasValidVoxelConfiguration() const
{
	return CellSize > 0 && CellNum > 0 && ChunkNum > 0;
}

int32 APlanet::CalculatePlanetDiameter() const
{
	const int64 CalculatedDiameter = static_cast<int64>(CellSize) * CellNum * ChunkNum;
	const int64 ClampedDiameter = FMath::Clamp<int64>(CalculatedDiameter, 0, TNumericLimits<int32>::Max());
	return static_cast<int32>(ClampedDiameter);
}

void APlanet::CacheVoxelSettingsFromManager()
{
	if (!VoxelManager)
	{
		return;
	}

	if (CellSize <= 0)
	{
		CellSize = VoxelManager->GetCellSize();
	}

	if (CellNum <= 0)
	{
		CellNum = VoxelManager->GetCellNum();
	}

	if (ChunkNum <= 0)
	{
		ChunkNum = VoxelManager->GetChunkNum();
	}
}

void APlanet::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	UpdatePlanetConfiguration();
}

