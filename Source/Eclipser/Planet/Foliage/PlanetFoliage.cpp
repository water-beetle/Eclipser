#include "PlanetFoliage.h"

#include "../Planet.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Planet/Voxel/VoxelChunk.h"
#include "Planet/Voxel/VoxelManager.h"

namespace
{
	bool GetSurfaceLocationAlong(const FVector& InDirection, FVector& OutLocation,
		const FPlanetSurfaceQueryData& SurfaceData,
		const FVector* ChunkCenter = nullptr, float ChunkHalfSize = 0.0f)
	{
		FVector Dir = InDirection.GetSafeNormal();
		if (Dir.IsNearlyZero())
		{
			return false;
		}

		const int32 BaseRadius = SurfaceData.PlanetRadius;
		if (BaseRadius <= 0)
		{
			return false;
		}

		const FPlanetNoiseSettings& Noise = SurfaceData.NoiseSettings;
		const bool bUseNoise = SurfaceData.bHasNoiseSettings && Noise.bEnableNoise && Noise.Octaves > 0;

		if (!bUseNoise)
		{
			OutLocation = SurfaceData.PlanetCenter + Dir * BaseRadius;
			return true;
		}

		const float InnerRadius = FMath::Max(0.0f, static_cast<float>(BaseRadius) - Noise.MaxDepression);
		const float OuterRadius = static_cast<float>(BaseRadius) + Noise.MaxRaise;

		if (InnerRadius >= OuterRadius)
		{
			OutLocation = SurfaceData.PlanetCenter + Dir * BaseRadius;
			return true;
		}

		const int32 MaxRadius = FMath::CeilToInt(static_cast<float>(BaseRadius) + Noise.MaxRaise);
		auto SampleDensity = [&](float R) -> float
		{
			const FVector LocalPos = Dir * R;
			return UVoxelChunk::CalculateDensity(LocalPos, BaseRadius, MaxRadius, &Noise);
		};

		float R0 = InnerRadius;
		float R1 = OuterRadius;
		float D0 = SampleDensity(R0);
		float D1 = SampleDensity(R1);

		if ((D0 > 0.f && D1 > 0.f) || (D0 < 0.f && D1 < 0.f))
		{
			OutLocation = SurfaceData.PlanetCenter + Dir * BaseRadius;
			return true;
		}

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
		OutLocation = SurfaceData.PlanetCenter + Dir * SurfaceR;

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
}

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
		RemoveInstancesFromComponent(GrassInstances, WorldCenter, SafeRadius, GrassConfig.bRemoveByDistance);
		RemoveInstancesFromComponent(FlowerInstances, WorldCenter, SafeRadius, Flowerconfig.bRemoveByDistance);
		RemoveInstancesFromComponent(TreeInstances, WorldCenter, SafeRadius, TreeConfig.bRemoveByDistance);
		RemoveInstancesFromComponent(RockInstances, WorldCenter, SafeRadius, RockConfig.bRemoveByDistance);
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

		RemoveInstancesFromComponent(Pair.Value.Grass, WorldCenter, SafeRadius, GrassConfig.bRemoveByDistance);
		RemoveInstancesFromComponent(Pair.Value.Flower, WorldCenter, SafeRadius, Flowerconfig.bRemoveByDistance);
		RemoveInstancesFromComponent(Pair.Value.Tree, WorldCenter, SafeRadius, TreeConfig.bRemoveByDistance);
		RemoveInstancesFromComponent(Pair.Value.Rock, WorldCenter, SafeRadius, RockConfig.bRemoveByDistance);
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

	FPlanetSurfaceQueryData SurfaceData;
	SurfaceData.PlanetCenter = PlanetCenter;
	SurfaceData.PlanetRadius = PlanetRadius;
	if (VoxelManager)
	{
		SurfaceData.NoiseSettings = VoxelManager->GetNoiseSettings();
		SurfaceData.bHasNoiseSettings = true;
	}

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

				auto EnqueueTask = [&](const FFoliageLayerConfig& Config,
				                       UHierarchicalInstancedStaticMeshComponent* TargetComponent)
				{
					if (!TargetComponent)
					{
						return;
					}

					if (!Config.Mesh)
					{
						ConfigureInstanceComponent(Config, TargetComponent);
						return;
					}

					const TWeakObjectPtr<UHierarchicalInstancedStaticMeshComponent> ComponentWeak = TargetComponent;

					Async(EAsyncExecution::ThreadPool,
					      [this, Config, ComponentWeak, ChunkRelativeCenter, ChunkHalfSize = CachedChunkHalfSize,
						      SurfaceData]()
					      {
						      TArray<FTransform> Transforms = GenerateTransformsForLayer(Config, ChunkRelativeCenter,
							      ChunkHalfSize, SurfaceData);

						      AsyncTask(ENamedThreads::GameThread,
						                [this, Config, ComponentWeak, Transforms = MoveTemp(Transforms)]() mutable
						                {
							                if (!ComponentWeak.IsValid())
							                {
								                return;
							                }

							                UHierarchicalInstancedStaticMeshComponent* Component = ComponentWeak.Get();

							                ConfigureInstanceComponent(Config, Component);

							                if (!Transforms.IsEmpty())
							                {
							                	for (const FTransform& T : Transforms)
							                	{
													Component->AddInstanceWorldSpace(T);
												}
							                }
						                });
					      });
				};

				EnqueueTask(GrassConfig, Entry.Grass);
				EnqueueTask(Flowerconfig, Entry.Flower);
				EnqueueTask(TreeConfig, Entry.Tree);
				EnqueueTask(RockConfig, Entry.Rock);

				Entry.ChunkCenterWorld = PlanetCenter + ChunkRelativeCenter;
				ChunkFoliageMap.Add(ChunkIndex, Entry);
			}
		}
	}
}

void UPlanetFoliage::RemoveInstancesFromComponent(UHierarchicalInstancedStaticMeshComponent* Instances,
	const FVector& WorldCenter, float Radius, bool bRemoveByDistance) const
{
	if (!Instances)
	{
		return;
	}

	// 거리기반
	if (bRemoveByDistance)
	{
		const float RadiusSq = Radius * Radius;
		const int32 InstanceCount = Instances->GetInstanceCount();

		if (InstanceCount <= 0)
		{
			return;
		}

		TArray<int32> RemoveIndices;
		RemoveIndices.Reserve(InstanceCount);

		for (int32 Index = 0; Index < InstanceCount; ++Index)
		{
			FTransform T;
			if (!Instances->GetInstanceTransform(Index, T, true))
			{
				continue;
			}

			const FVector Loc = T.GetLocation();
			if (FVector::DistSquared(Loc, WorldCenter) <= RadiusSq)
			{
				RemoveIndices.Add(Index);
			}
		}

		if (RemoveIndices.IsEmpty())
		{
			return;
		}

		// 인덱스가 밀리지 않게 뒤에서부터 제거
		RemoveIndices.Sort();
		for (int32 i = RemoveIndices.Num() - 1; i >= 0; --i)
		{
			Instances->RemoveInstance(RemoveIndices[i]);
		}

		return;
	}

	// 바운딩박스 기반
	TArray<int32> InstanceIndices =
		Instances->GetInstancesOverlappingSphere(WorldCenter, Radius, true);

	if (InstanceIndices.IsEmpty())
	{
		return;
	}

	Instances->RemoveInstances(InstanceIndices);
}

TArray<FTransform> UPlanetFoliage::GenerateTransformsForLayer(const FFoliageLayerConfig& Config, const FVector& ChunkRelativeCenter,
		float ChunkHalfSize, const FPlanetSurfaceQueryData& SurfaceData) const
{
	TArray<FTransform> Result;

	if (!Config.Mesh)
	{
		return Result;
	}

	if (SurfaceData.PlanetRadius <= KINDA_SMALL_NUMBER)
	{
		return Result;
	}

	const FVector SafeChunkCenter = ChunkRelativeCenter;
	const float SafeHalfSize = FMath::Max(0.0f, ChunkHalfSize);
	const FVector ChunkCenterWorld = SurfaceData.PlanetCenter + SafeChunkCenter;

	const int32 DesiredInstanceCount = Config.InstanceCount;
	const int32 DesiredClusterCount = FMath::Max(1, Config.ClusterCount);
	const float ClampedSpread = FMath::Max(0.0f, Config.ClusterSpread);

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
			if (!GetSurfaceLocationAlong(Direction, SurfaceLocation, SurfaceData, &ChunkCenterWorld, SafeHalfSize))
			{
				continue;
			}

			const FVector Normal = (SurfaceLocation - SurfaceData.PlanetCenter).GetSafeNormal();
			const FVector Location = SurfaceLocation + Normal * Config.SurfaceOffset;

			const FQuat Alignment = FRotationMatrix::MakeFromZ(Normal).ToQuat();
			const FQuat RandomTwist = FQuat(Normal, FMath::FRandRange(0.0f, TWO_PI));

			FTransform InstanceTransform(RandomTwist * Alignment);
			InstanceTransform.SetLocation(Location);
			InstanceTransform.SetScale3D(FVector(CalculateRandomScale(Config)));

			Result.Add(InstanceTransform);
		}
	}

	return Result;
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
