#include "PlanetFoliage.h"

#include "../Planet.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Engine/CollisionProfile.h"

UPlanetFoliage::UPlanetFoliage()
{
	PrimaryComponentTick.bCanEverTick = false;

	GrassInstances = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("GrassInstances"));
	GrassInstances->SetupAttachment(this);

	FlowerInstances = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("FlowerInstances"));
	FlowerInstances->SetupAttachment(this);
	
	TreeInstances = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("TreeInstances"));
	TreeInstances->SetupAttachment(this);

	RockInstances = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("RockInstances"));
	RockInstances->SetupAttachment(this);

	GrassConfig.bEnableCollision = false;
	GrassConfig.bAffectNavigation = false;

	Flowerconfig.bEnableCollision = false;
	Flowerconfig.bAffectNavigation = false;
	
	TreeConfig.InstanceCount = 2000;
	TreeConfig.ClusterCount = 150;
	TreeConfig.MinUniformScale = 0.8f;
	TreeConfig.MaxUniformScale = 1.5f;
	TreeConfig.bEnableCollision = true;
	TreeConfig.bAffectNavigation = true;

	RockConfig.InstanceCount = 3000;
	RockConfig.ClusterCount = 200;
	RockConfig.MinUniformScale = 0.7f;
	RockConfig.MaxUniformScale = 1.3f;
	RockConfig.bEnableCollision = true;
	RockConfig.bAffectNavigation = true;
}

void UPlanetFoliage::RemoveInstancesWithinRadius(const FVector& WorldCenter, float Radius)
{
	const float SafeRadius = FMath::Max(0.0f, Radius);
	if (SafeRadius <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	RemoveInstancesFromComponent(GrassInstances, WorldCenter, SafeRadius);
	RemoveInstancesFromComponent(FlowerInstances, WorldCenter, SafeRadius);
	RemoveInstancesFromComponent(TreeInstances, WorldCenter, SafeRadius);
	RemoveInstancesFromComponent(RockInstances, WorldCenter, SafeRadius);
}

void UPlanetFoliage::BeginPlay()
{
	Super::BeginPlay();
	GenerateFoliageInstances();
}


void UPlanetFoliage::GenerateFoliageInstances()
{
	GenerateInstancesForLayer(GrassConfig, GrassInstances);
	GenerateInstancesForLayer(Flowerconfig, FlowerInstances);
	GenerateInstancesForLayer(TreeConfig, TreeInstances);
	GenerateInstancesForLayer(RockConfig, RockInstances);
}

void UPlanetFoliage::RemoveInstancesFromComponent(UHierarchicalInstancedStaticMeshComponent* Instances,
	const FVector& WorldCenter, float Radius) const
{
	if (!Instances)
	{
		return;
	}

	TArray<int32> InstanceIndices = Instances->GetInstancesOverlappingSphere(WorldCenter, Radius, true);

	if (InstanceIndices.IsEmpty())
	{
		return;
	}

	Instances->RemoveInstances(InstanceIndices);
}

void UPlanetFoliage::GenerateInstancesForLayer(const FFoliageLayerConfig& Config,
                                               UHierarchicalInstancedStaticMeshComponent* Instances)
{
	if (!Instances)
	{
		return;
	}

	ConfigureInstanceComponent(Config, Instances);

	if (!Config.Mesh)
	{
		Instances->ClearInstances();
		return;
	}

	APlanet* PlanetActor = ResolvePlanetActor();
	const float PlanetRadius = ResolvePlanetRadius(PlanetActor);
	if (PlanetRadius <= KINDA_SMALL_NUMBER || !PlanetActor)
	{
		Instances->ClearInstances();
		return;
	}

	const FVector PlanetCenter = ResolvePlanetCenter(PlanetActor);
	const int32 DesiredInstanceCount = FMath::Max(0, Config.InstanceCount);
	const int32 DesiredClusterCount = FMath::Max(1, Config.ClusterCount);
	const float ClampedSpread = FMath::Max(0.0f, Config.ClusterSpread);

	Instances->ClearInstances();

	const int32 BaseClusterSize = DesiredInstanceCount / DesiredClusterCount;
	int32 Remainder = DesiredInstanceCount % DesiredClusterCount;

	for (int32 ClusterIndex = 0; ClusterIndex < DesiredClusterCount; ++ClusterIndex)
	{
		const int32 InstancesInCluster = BaseClusterSize + (Remainder-- > 0 ? 1 : 0);
		if (InstancesInCluster <= 0)
			continue;

		const FVector ClusterDirection = FMath::VRand().GetSafeNormal();

		for (int32 InstanceIndex = 0; InstanceIndex < InstancesInCluster; ++InstanceIndex)
		{
			FVector Direction = (ClusterDirection + (ClampedSpread * FMath::VRand())).GetSafeNormal();
			if (Direction.IsNearlyZero())
				Direction = ClusterDirection;

			FVector SurfaceLocation;
			if (!PlanetActor->GetSurfaceLocationAlong(Direction, SurfaceLocation))
			{
				SurfaceLocation = PlanetCenter + Direction * (PlanetRadius);
			}

			const FVector Normal = (SurfaceLocation - PlanetCenter).GetSafeNormal();
			const FVector Location = SurfaceLocation + Normal * Config.SurfaceOffset;

			const FQuat Alignment = FRotationMatrix::MakeFromZ(Normal).ToQuat();
			const FQuat RandomTwist = FQuat(Normal, FMath::FRandRange(0.0f, TWO_PI));

			FTransform InstanceTransform(RandomTwist * Alignment);
			InstanceTransform.SetLocation(Location);
			InstanceTransform.SetScale3D(FVector(CalculateRandomScale(Config)));

			Instances->AddInstanceWorldSpace(InstanceTransform);
		}
	}
}

void UPlanetFoliage::ConfigureInstanceComponent(const FFoliageLayerConfig& Config,
                                                UHierarchicalInstancedStaticMeshComponent* Instances) const
{
	if (!Instances)
	{
		return;
	}

	Instances->SetStaticMesh(Config.Mesh);
	Instances->SetCollisionProfileName(Config.bEnableCollision
		                                   ? UCollisionProfile::BlockAll_ProfileName
		                                   : UCollisionProfile::NoCollision_ProfileName);
	Instances->SetCollisionEnabled(Config.bEnableCollision
		                               ? ECollisionEnabled::QueryAndPhysics
		                               : ECollisionEnabled::NoCollision);
	Instances->SetGenerateOverlapEvents(Config.bEnableCollision);
	Instances->SetCanEverAffectNavigation(Config.bEnableCollision || Config.bAffectNavigation);
	Instances->SetMobility(EComponentMobility::Static);
}

APlanet* UPlanetFoliage::ResolvePlanetActor() const
{
	if (PlanetOverride)
	{
		return PlanetOverride;
	}

	return Cast<APlanet>(GetOwner());
}

float UPlanetFoliage::ResolvePlanetRadius(const APlanet* PlanetActor) const
{
	if (PlanetActor)
	{
		return PlanetActor->GetPlanetRadius();
	}

	return 0.0f;
}

FVector UPlanetFoliage::ResolvePlanetCenter(const APlanet* PlanetActor) const
{
	if (PlanetActor)
	{
		return PlanetActor->GetActorLocation();
	}

	return GetComponentLocation();
}

float UPlanetFoliage::CalculateRandomScale(const FFoliageLayerConfig& Config) const
{
	const float SafeMinScale = FMath::Max(0.0f, Config.MinUniformScale);
	const float SafeMaxScale = FMath::Max(SafeMinScale, Config.MaxUniformScale);
	return FMath::FRandRange(SafeMinScale, SafeMaxScale);
}
