#include "PlanetGrass.h"

#include "../Planet.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"

UPlanetGrass::UPlanetGrass()
{
	PrimaryComponentTick.bCanEverTick = false;

	GrassInstances = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("GrassInstances"));
	GrassInstances->SetupAttachment(this);
	GrassInstances->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GrassInstances->SetMobility(EComponentMobility::Static);
}

void UPlanetGrass::OnRegister()
{
	Super::OnRegister();

	if (bRegenerateOnRegister)
	{
		GenerateGrassInstances();
	}
}

void UPlanetGrass::BeginPlay()
{
	Super::BeginPlay();

	if (!bRegenerateOnRegister)
	{
		GenerateGrassInstances();
	}
}

void UPlanetGrass::RegenerateGrassInstances()
{
	GenerateGrassInstances();
}

#if WITH_EDITOR
void UPlanetGrass::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	GenerateGrassInstances();
}
#endif

void UPlanetGrass::GenerateGrassInstances()
{
	if (!GrassInstances || !GrassInstances->GetStaticMesh())
	{
		return;
	}

	APlanet* PlanetActor = ResolvePlanetActor();
	const float PlanetRadius = ResolvePlanetRadius(PlanetActor);
	if (PlanetRadius <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	const FVector PlanetCenter = ResolvePlanetCenter(PlanetActor);
	const int32 DesiredInstanceCount = FMath::Max(0, InstanceCount);
	const int32 DesiredClusterCount = FMath::Max(1, ClusterCount);
	const float ClampedSpread = FMath::Max(0.0f, ClusterSpread);

	GrassInstances->ClearInstances();

	const int32 BaseClusterSize = DesiredInstanceCount / DesiredClusterCount;
	int32 Remainder = DesiredInstanceCount % DesiredClusterCount;

	for (int32 ClusterIndex = 0; ClusterIndex < DesiredClusterCount; ++ClusterIndex)
	{
		const int32 InstancesInCluster = BaseClusterSize + (Remainder-- > 0 ? 1 : 0);
		if (InstancesInCluster <= 0)
		{
			continue;
		}

		const FVector ClusterDirection = FMath::VRand().GetSafeNormal();

		for (int32 InstanceIndex = 0; InstanceIndex < InstancesInCluster; ++InstanceIndex)
		{
			FVector Direction = (ClusterDirection + (ClampedSpread * FMath::VRand())).GetSafeNormal();
			if (Direction.IsNearlyZero())
			{
				Direction = ClusterDirection;
			}

			const FVector Normal = Direction.GetSafeNormal();
			const FVector Location = PlanetCenter + Normal * (PlanetRadius + SurfaceOffset);
			const FQuat Alignment = FRotationMatrix::MakeFromZ(Normal).ToQuat();
			const FQuat RandomTwist = FQuat(Normal, FMath::FRandRange(0.0f, TWO_PI));
			FTransform InstanceTransform(RandomTwist * Alignment);
			InstanceTransform.SetLocation(Location);
			InstanceTransform.SetScale3D(FVector(CalculateRandomScale()));

			GrassInstances->AddInstanceWorldSpace(InstanceTransform);
		}
	}
}

APlanet* UPlanetGrass::ResolvePlanetActor() const
{
	if (PlanetOverride)
	{
		return PlanetOverride;
	}

	return Cast<APlanet>(GetOwner());
}

float UPlanetGrass::ResolvePlanetRadius(const APlanet* PlanetActor) const
{
	if (PlanetActor)
	{
		return PlanetActor->GetPlanetRadius();
	}

	return 0.0f;
}

FVector UPlanetGrass::ResolvePlanetCenter(const APlanet* PlanetActor) const
{
	if (PlanetActor)
	{
		return PlanetActor->GetActorLocation();
	}

	return GetComponentLocation();
}

float UPlanetGrass::CalculateRandomScale() const
{
	const float SafeMinScale = FMath::Max(0.0f, MinUniformScale);
	const float SafeMaxScale = FMath::Max(SafeMinScale, MaxUniformScale);
	return FMath::FRandRange(SafeMinScale, SafeMaxScale);
}
