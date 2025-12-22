// Fill out your copyright notice in the Description page of Project Settings.


#include "Planet.h"

#include "Gravity/GravityFieldCenter.h"
#include "Foliage/PlanetFoliage.h"
#include "Voxel/VoxelChunk.h"
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
	FoliageComponent = CreateDefaultSubobject<UPlanetFoliage>(TEXT("PlanetFoliage"));
	FoliageComponent->SetupAttachment(PlanetRoot);
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

	const FPlanetNoiseSettings* NoiseSettings = VoxelManager ? &VoxelManager->GetNoiseSettings() : nullptr;
	const int32 MaxSurfaceOffset = (NoiseSettings && NoiseSettings->bEnableNoise)
			? FMath::CeilToInt(FMath::Max(NoiseSettings->MaxRaise, 0.0f))
			: 0;

	const int32 MaxAllowedRadius = PlanetDiameter / 2 - MaxSurfaceOffset;
	PlanetRadius = FMath::Clamp(PlanetRadius, 0, FMath::Max(0, MaxAllowedRadius));

	const int32 SurfaceRadius = PlanetRadius + MaxSurfaceOffset;

	if (GravityField)
	{
		GravityField->SetGravityFieldSize(SurfaceRadius * 1.2f);
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

bool APlanet::GetSurfaceLocationAlong(const FVector& InDirection, FVector& OutLocation,
		const FVector* ChunkCenter, float ChunkHalfSize) const
{
	if (!VoxelManager)
	{
		return false;
	}

	FVector Dir = InDirection.GetSafeNormal();
	if (Dir.IsNearlyZero())
	{
		return false;
	}

	const int32 BaseRadius = PlanetRadius;
	const FPlanetNoiseSettings& Noise = VoxelManager->GetNoiseSettings();
    
	// 노이즈 없으면 그냥 구
	if (!Noise.bEnableNoise || Noise.Octaves <= 0)
	{
		OutLocation = GetActorLocation() + Dir * BaseRadius;
		return true;
	}

	// 밀고 파인 깊이 범위를 이용해서 이분 탐색 구간 잡기
	const float InnerRadius = FMath::Max(0.0f, BaseRadius - Noise.MaxDepression);
	const float OuterRadius = BaseRadius + Noise.MaxRaise;

	if (InnerRadius >= OuterRadius)
	{
		OutLocation = GetActorLocation() + Dir * BaseRadius;
		return true;
	}

	const int32   MaxRadius = BaseRadius + Noise.MaxRaise;
	auto SampleDensity = [&](float R) -> float
	{
		const FVector LocalPos = Dir * R; // Planet의 로컬 좌표: Voxel도 Planet 기준으로 생성하니까
		return UVoxelChunk::CalculateDensity(LocalPos, BaseRadius, MaxRadius, &Noise);
	};

	float R0 = InnerRadius;
	float R1 = OuterRadius;
	float D0 = SampleDensity(R0);
	float D1 = SampleDensity(R1);

	// 둘 다 안 or 밖이면 실패 (fallback)
	if ((D0 > 0.f && D1 > 0.f) || (D0 < 0.f && D1 < 0.f))
	{
		OutLocation = GetActorLocation() + Dir * BaseRadius;
		return true;
	}

	// 이분 탐색
	for (int i = 0; i < 8; ++i)
	{
		const float MidR = 0.5f * (R0 + R1);
		const float DMid = SampleDensity(MidR);

		if ((D0 > 0.f && DMid > 0.f) || (D0 < 0.f && DMid < 0.f))
		{
			R0 = MidR;
			D0 = DMid;
		}
		else
		{
			R1 = MidR;
			D1 = DMid;
		}
	}

	const float SurfaceR = 0.5f * (R0 + R1);
	OutLocation = GetActorLocation() + Dir * SurfaceR;

	if (ChunkCenter && ChunkHalfSize > KINDA_SMALL_NUMBER)
	{
		const FVector HalfExtent(ChunkHalfSize);
		const FBox ChunkBounds(*ChunkCenter - HalfExtent, *ChunkCenter + HalfExtent);

		if (!ChunkBounds.IsInside(OutLocation))
		{
			return false;
		}
	}

	
	return true;
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

