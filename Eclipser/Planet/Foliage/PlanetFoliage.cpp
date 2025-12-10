#include "PlanetFoliage.h"

#include "../Planet.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Planet/Voxel/VoxelManager.h"

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

	const float ChunkRadius = CachedChunkHalfSize > 0.0f
								  ? CachedChunkHalfSize * FMath::Sqrt(3.0f)
								  : 0.0f;

	if (ChunkFoliageMap.Num() == 0 || ChunkRadius <= KINDA_SMALL_NUMBER)
	{
		RemoveInstancesFromComponent(GrassInstances, WorldCenter, SafeRadius);
		RemoveInstancesFromComponent(FlowerInstances, WorldCenter, SafeRadius);
		RemoveInstancesFromComponent(TreeInstances, WorldCenter, SafeRadius);
		RemoveInstancesFromComponent(RockInstances, WorldCenter, SafeRadius);
		return;
	}

	// 최대 범위 안에 포함되는 Chunk에서만 Foliage 삭제
	for (const auto& Pair : ChunkFoliageMap)
	{
		const FVector ChunkCenter = Pair.Value.ChunkCenterWorld;
		if (ChunkRadius > 0.0f)
		{
			const float DistanceToChunk = FVector::Dist(ChunkCenter, WorldCenter);
			if (DistanceToChunk > SafeRadius + ChunkRadius)
			{
				continue;
			}
		}

		RemoveInstancesFromComponent(Pair.Value.Grass, WorldCenter, SafeRadius);
		RemoveInstancesFromComponent(Pair.Value.Flower, WorldCenter, SafeRadius);
		RemoveInstancesFromComponent(Pair.Value.Tree, WorldCenter, SafeRadius);
		RemoveInstancesFromComponent(Pair.Value.Rock, WorldCenter, SafeRadius);
	}
}

void UPlanetFoliage::BeginPlay()
{
	Super::BeginPlay();
	GenerateFoliageInstances();
}


void UPlanetFoliage::GenerateFoliageInstances()
{
	ClearChunkFoliage();

	APlanet* PlanetActor = ResolvePlanetActor();
	const UVoxelManager* VoxelManager = ResolveVoxelManager();

	const float PlanetRadius = ResolvePlanetRadius(PlanetActor);
	if (PlanetRadius <= KINDA_SMALL_NUMBER || !PlanetActor)
	{
		return;
	}

	const FVector PlanetCenter = ResolvePlanetCenter(PlanetActor);
	const int32 ChunkNum = ResolveChunkNum(PlanetActor, VoxelManager);
	const int32 CellSize = ResolveCellSize(PlanetActor, VoxelManager);
	const int32 CellNum = ResolveCellNum(PlanetActor, VoxelManager);

	const int32 ChunkSize = CellSize * CellNum;
	CachedChunkHalfSize = static_cast<float>(ChunkSize) * 0.5f;

	const int32 TotalChunkCount = FMath::Max(1, ChunkNum * ChunkNum * ChunkNum);

	for (int32 X = 0; X < ChunkNum; ++X)
	{
		for (int32 Y = 0; Y < ChunkNum; ++Y)
		{
			for (int32 Z = 0; Z < ChunkNum; ++Z)
			{
				const FIntVector ChunkIndex(X, Y, Z);
				const FVector ChunkRelativeCenter = (FVector(ChunkIndex) + 0.5f) * ChunkSize -
					FVector(ChunkSize * ChunkNum * 0.5f);

				FChunkFoliageComponents Entry;
				Entry.Grass = CreateChunkComponent(TEXT("GrassInstances"));
				Entry.Flower = CreateChunkComponent(TEXT("FlowerInstances"));
				Entry.Tree = CreateChunkComponent(TEXT("TreeInstances"));
				Entry.Rock = CreateChunkComponent(TEXT("RockInstances"));

				if (Entry.Grass)
				{
					Entry.Grass->SetRelativeLocation(ChunkRelativeCenter);
				}
				if (Entry.Flower)
				{
					Entry.Flower->SetRelativeLocation(ChunkRelativeCenter);
				}
				if (Entry.Tree)
				{
					Entry.Tree->SetRelativeLocation(ChunkRelativeCenter);
				}
				if (Entry.Rock)
				{
					Entry.Rock->SetRelativeLocation(ChunkRelativeCenter);
				}

				GenerateInstancesForLayer(GrassConfig, Entry.Grass, ChunkRelativeCenter, CachedChunkHalfSize,
				                          TotalChunkCount, PlanetCenter, PlanetRadius, PlanetActor);
				GenerateInstancesForLayer(Flowerconfig, Entry.Flower, ChunkRelativeCenter, CachedChunkHalfSize,
				                          TotalChunkCount, PlanetCenter, PlanetRadius, PlanetActor);
				GenerateInstancesForLayer(TreeConfig, Entry.Tree, ChunkRelativeCenter, CachedChunkHalfSize,
				                          TotalChunkCount, PlanetCenter, PlanetRadius, PlanetActor);
				GenerateInstancesForLayer(RockConfig, Entry.Rock, ChunkRelativeCenter, CachedChunkHalfSize,
				                          TotalChunkCount, PlanetCenter, PlanetRadius, PlanetActor);

				Entry.ChunkCenterWorld = PlanetCenter + ChunkRelativeCenter;
				ChunkFoliageMap.Add(ChunkIndex, Entry);
			}
		}
	}
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
	UHierarchicalInstancedStaticMeshComponent* Instances, const FVector& ChunkRelativeCenter, float ChunkHalfSize,
	int32 TotalChunkCount, const FVector& PlanetCenter, float PlanetRadius, APlanet* PlanetActor)
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

	if (PlanetRadius <= KINDA_SMALL_NUMBER || !PlanetActor)
	{
		Instances->ClearInstances();
		return;
	}

	const FVector SafeChunkCenter = ChunkRelativeCenter;
	const float SafeHalfSize = FMath::Max(0.0f, ChunkHalfSize);

	const int32 DesiredInstanceCount = TotalChunkCount > 0
												   ? FMath::CeilToInt(static_cast<float>(Config.InstanceCount) / TotalChunkCount)
												   : Config.InstanceCount;
	const int32 DesiredClusterCountRaw = TotalChunkCount > 0
												 ? FMath::CeilToInt(static_cast<float>(Config.ClusterCount) / TotalChunkCount)
												 : Config.ClusterCount;
	const int32 DesiredClusterCount = FMath::Max(1, DesiredClusterCountRaw);
	const float ClampedSpread = FMath::Max(0.0f, Config.ClusterSpread);

	Instances->ClearInstances();

	const int32 BaseClusterSize = DesiredInstanceCount / DesiredClusterCount;
	int32 Remainder = DesiredInstanceCount % DesiredClusterCount;

	for (int32 ClusterIndex = 0; ClusterIndex < DesiredClusterCount; ++ClusterIndex)
	{
		const int32 InstancesInCluster = BaseClusterSize + (Remainder-- > 0 ? 1 : 0);
		if (InstancesInCluster <= 0)
			continue;

		const FVector RandomChunkPoint = SafeChunkCenter + FVector(FMath::FRandRange(-SafeHalfSize, SafeHalfSize),
																   FMath::FRandRange(-SafeHalfSize, SafeHalfSize),
																   FMath::FRandRange(-SafeHalfSize, SafeHalfSize));
		FVector ClusterDirection = RandomChunkPoint.GetSafeNormal();
		if (ClusterDirection.IsNearlyZero())
		{
			ClusterDirection = FMath::VRand().GetSafeNormal();
		}

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

const UVoxelManager* UPlanetFoliage::ResolveVoxelManager() const
{
	if (const APlanet* Planet = ResolvePlanetActor())
	{
		return Planet->VoxelManager.Get();
	}

	return nullptr;
}

int32 UPlanetFoliage::ResolveChunkNum(const APlanet* PlanetActor, const UVoxelManager* VoxelManager) const
{
	if (VoxelManager && VoxelManager->GetChunkNum() > 0)
	{
		return VoxelManager->GetChunkNum();
	}

	return PlanetActor ? FMath::Max(1, PlanetActor->ChunkNum) : 1;
}

int32 UPlanetFoliage::ResolveCellSize(const APlanet* PlanetActor, const UVoxelManager* VoxelManager) const
{
	if (VoxelManager && VoxelManager->GetCellSize() > 0)
	{
		return VoxelManager->GetCellSize();
	}

	return PlanetActor ? FMath::Max(1, PlanetActor->CellSize) : 1;
}

int32 UPlanetFoliage::ResolveCellNum(const APlanet* PlanetActor, const UVoxelManager* VoxelManager) const
{
	if (VoxelManager && VoxelManager->GetCellNum() > 0)
	{
		return VoxelManager->GetCellNum();
	}

	return PlanetActor ? FMath::Max(1, PlanetActor->CellNum) : 1;
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

void UPlanetFoliage::ClearChunkFoliage()
{
	CachedChunkHalfSize = 0.0f;

	for (TObjectPtr<UHierarchicalInstancedStaticMeshComponent>& Component : SpawnedChunkComponents)
	{
		if (Component)
		{
			Component->DestroyComponent();
		}
	}
	SpawnedChunkComponents.Reset();
	ChunkFoliageMap.Reset();

	if (GrassInstances)
	{
		GrassInstances->ClearInstances();
		GrassInstances->SetRelativeLocation(FVector::ZeroVector);
	}
	if (FlowerInstances)
	{
		FlowerInstances->ClearInstances();
		FlowerInstances->SetRelativeLocation(FVector::ZeroVector);
	}
	if (TreeInstances)
	{
		TreeInstances->ClearInstances();
		TreeInstances->SetRelativeLocation(FVector::ZeroVector);
	}
	if (RockInstances)
	{
		RockInstances->ClearInstances();
		RockInstances->SetRelativeLocation(FVector::ZeroVector);
	}
}

UHierarchicalInstancedStaticMeshComponent* UPlanetFoliage::CreateChunkComponent(const FString& BaseName)
{
	const FString UniqueName = FString::Printf(TEXT("%s_%d"), *BaseName, SpawnedChunkComponents.Num());
	UHierarchicalInstancedStaticMeshComponent* NewComponent = NewObject<UHierarchicalInstancedStaticMeshComponent>(GetOwner(), *UniqueName);
	if (!NewComponent)
	{
		return nullptr;
	}

	NewComponent->SetupAttachment(this);
	NewComponent->RegisterComponent();
	NewComponent->SetMobility(EComponentMobility::Movable);

	SpawnedChunkComponents.Add(NewComponent);
	return NewComponent;
}
