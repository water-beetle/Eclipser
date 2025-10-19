// Fill out your copyright notice in the Description page of Project Settings.

#include "VoxelManager.h"
#include "Planet/Voxel/VoxelChunk.h"
#include "Defines/VoxelStructs.h"
#include "etc/VoxelHelper.h"
#include "Kismet/GameplayStatics.h"


// Sets default values for this component's properties
UVoxelManager::UVoxelManager()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


void UVoxelManager::GenerateChunk()
{
	check(GetOwner());

	if (ChunkNum <= 0 || CellNum <= 0 || CellSize <= 0) return;

	Algo::SortBy(LODDistanceLevels, &FLODDistanceLevel::DistanceThreshold);

	TArray<FChunkGenerationRequest> GenerationRequests;
	GenerationRequests.Reserve(ChunkNum * ChunkNum * ChunkNum);
	
	// Voxel은 Actor의 Location을 중점으로 생성됨
	for (int32 x = 0; x < ChunkNum; ++x)
		for (int32 y = 0; y < ChunkNum; ++y)
			for (int32 z = 0; z < ChunkNum; ++z)
			{
				FChunkSettingInfo ChunkInfo{ FIntVector(x,y,z), CellSize, CellNum, ChunkNum, 1};
				ChunkInfo.Calculate();
				
				UVoxelChunk* Chunk = NewObject<UVoxelChunk>(GetOwner());
				Chunk->RegisterComponent();
				Chunk->AttachToComponent(this, FAttachmentTransformRules::KeepRelativeTransform);
				Chunk->SetRelativeLocation(ChunkInfo.ChunkPos);

				Chunk->InitializeChunk((ChunkInfo));
				Chunk->SetVoxelManager(this);

				RegisterChunk(FIntVector(x,y,z), Chunk);

				FChunkGenerationRequest& Request = GenerationRequests.Emplace_GetRef();
				Request.Chunk = Chunk;
				Request.Info = ChunkInfo;
				const FVector ChunkWorldLocation = GetComponentTransform().TransformPosition(ChunkInfo.ChunkPos);
				Request.DistanceSquared = FVector::DistSquared(GetReferenceLocation(), ChunkWorldLocation);
				const float Distance = FMath::Sqrt(Request.DistanceSquared);
				Request.Info.LODLevel = ComputeLODLevel(Distance);
				Request.Info.Calculate();
			}

	GenerationRequests.Shrink();
	Algo::SortBy(GenerationRequests, &FChunkGenerationRequest::DistanceSquared);

	for (FChunkGenerationRequest& Request : GenerationRequests)
	{
		EnqueueGenerateChunk(Request.Chunk, Request.Info);
		++TotalChunkCount;
	}
}

// Called when the game starts
void UVoxelManager::BeginPlay()
{
	Super::BeginPlay();

	GenerateChunk();
}


// Called every frame
void UVoxelManager::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	TimeSinceLastLODUpdate += DeltaTime;

	const bool bShouldUpdateLOD = LODUpdateInterval <= 0.0f || TimeSinceLastLODUpdate >= LODUpdateInterval;
	if (bShouldUpdateLOD)
	{
		const FVector ReferenceLocation = GetReferenceLocation();
		UpdateChunkLODLevels(ReferenceLocation);
		if (LODUpdateInterval > 0.0f)
		{
			TimeSinceLastLODUpdate = 0.0f;
		}
	}
	
	GenerateCompletedChunk();
}

void UVoxelManager::RegisterChunk(const FIntVector& Index, UVoxelChunk* Chunk)
{
	ChunkMap.Add(Index, Chunk);
}

UVoxelChunk* UVoxelManager::GetChunk(const FIntVector& Index)
{
	if (UVoxelChunk** Ptr = ChunkMap.Find(Index))
	{
		return *Ptr;
	}
	return nullptr;
}

void UVoxelManager::Sculpt(const FVector& ImpactPoint, float Radius)
{
	// Sculpt 지점과, 반지름에 영향을 받는 Chunk만 다시 Density 계산 후 mesh 재생성
	
	if (ChunkNum <= 0 || CellNum <= 0 || CellSize <= 0)
		return;

	const int ChunkSize = CellSize * CellNum;
	const float VoxelSize = ChunkSize * ChunkNum;
	const FVector VoxelMinCorner = GetComponentLocation() - FVector(VoxelSize) * 0.5f;

	const FVector SculptMin = ImpactPoint - FVector(Radius);
	const FVector SculptMax = ImpactPoint + FVector(Radius);

	auto ComputeMinIndex = [&](float Coordinate, int32 Axis) -> int32
	{
		const float Origin = (&VoxelMinCorner.X)[Axis];
		const float Normalized = (Coordinate - Origin) / ChunkSize;
		return FMath::Clamp(FMath::FloorToInt(Normalized), 0, ChunkNum - 1);
	};

	auto ComputeMaxIndex = [&](float Coordinate, int32 Axis) -> int32
	{
		const float Origin = VoxelMinCorner[Axis];
		const float Normalized = (Coordinate - Origin) / ChunkSize;
		return FMath::Clamp(FMath::CeilToInt(Normalized) - 1, 0, ChunkNum - 1);
	};

	const int32 StartX = ComputeMinIndex(SculptMin.X, 0);
	const int32 StartY = ComputeMinIndex(SculptMin.Y, 1);
	const int32 StartZ = ComputeMinIndex(SculptMin.Z, 2);

	const int32 EndX = ComputeMaxIndex(SculptMax.X, 0);
	const int32 EndY = ComputeMaxIndex(SculptMax.Y, 1);
	const int32 EndZ = ComputeMaxIndex(SculptMax.Z, 2);

	for (int32 x = StartX; x <= EndX; ++x)
	{
		for (int32 y = StartY; y <= EndY; ++y)
		{
			for (int32 z = StartZ; z <= EndZ; ++z)
			{
				if (UVoxelChunk* Chunk = GetChunk(FIntVector(x, y, z)))
				{
					Chunk->Sculpt(ImpactPoint, Radius);
				}
			}
		}
	}
}

void UVoxelManager::RecordSculptedDensity(const FChunkSettingInfo& Info, int32 LocalX, int32 LocalY, int32 LocalZ, float Density)
{
	FScopeLock Lock(&SculptedDensityLock);
	UVoxelManager::FChunkSculptOverrides& ChunkOverrides = SculptedDensityMap.FindOrAdd(Info.ChunkIndex);
	const int32 VertexIndex = VoxelHelper::GetIndex(LocalX, LocalY, LocalZ, Info.CellNum);

	ChunkOverrides.VertexDensities.Add(VertexIndex, FFloat16(Density));
}

void UVoxelManager::ApplySculptedDensityOverrides(const FChunkSettingInfo& Info, TArray<FVertexDensity>& DensityData)
{
	if (DensityData.Num() == 0)
	{
		return;
	}

	FScopeLock Lock(&SculptedDensityLock);
	const UVoxelManager::FChunkSculptOverrides* ChunkOverrides = SculptedDensityMap.Find(Info.ChunkIndex);
	if (!ChunkOverrides || ChunkOverrides->VertexDensities.Num() == 0)
	{
		return;
	}

	TArray<int32> KeysToRemove;
	KeysToRemove.Reserve(ChunkOverrides->VertexDensities.Num());

	for (const TPair<int32, FFloat16>& Override : ChunkOverrides->VertexDensities)
	{
		if (!DensityData.IsValidIndex(Override.Key))
		{
			KeysToRemove.Add(Override.Key);
			continue;
		}

		float& BaseDensity = DensityData[Override.Key].Density;
		const float OverrideDensity = static_cast<float>(Override.Value);

		if (FMath::IsNearlyEqual(BaseDensity, OverrideDensity))
		{
			KeysToRemove.Add(Override.Key);
			continue;
		}

		BaseDensity = OverrideDensity;
	}

	if (KeysToRemove.Num() > 0)
	{
		if (UVoxelManager::FChunkSculptOverrides* MutableOverrides = SculptedDensityMap.Find(Info.ChunkIndex))
		{
			for (int32 Key : KeysToRemove)
			{
				MutableOverrides->VertexDensities.Remove(Key);
			}

			if (MutableOverrides->VertexDensities.Num() == 0)
			{
				SculptedDensityMap.Remove(Info.ChunkIndex);
			}
		}
	}
}


void UVoxelManager::EnqueueGenerateChunk(UVoxelChunk* Chunk, const FChunkSettingInfo& ChunkInfo)
{
	if (!IsValid(Chunk)) return;

	Chunk->SetRequestedLODLevel(ChunkInfo.LODLevel);
	
	TWeakObjectPtr<UVoxelManager> ManagerPtr(this);
	TWeakObjectPtr<UVoxelChunk> ChunkPtr(Chunk);

	UE::Tasks::Launch(
		UE_SOURCE_LOCATION, [ManagerPtr, ChunkPtr, ChunkInfo]()
		{
			UVoxelManager* Manager = ManagerPtr.Get();
			FChunkBuildResult Result = UVoxelChunk::GenerateChunkData(ChunkInfo, Manager);

			if (Manager)
			{
				   Manager->PushCompletedResult(MoveTemp(Result), ChunkPtr, ChunkInfo);
			}
		},
	UE::Tasks::ETaskPriority::BackgroundHigh
	);
}

void UVoxelManager::GenerateCompletedChunk()
{
	FPendingChunkResult PendingResult;
	const double StartTime = FPlatformTime::Seconds();
	int32 ProcessedCount = 0;
	const double TimeBudgetSeconds = ChunkProcessingTimeBudgetMs > 0.0f
											 ? static_cast<double>(ChunkProcessingTimeBudgetMs) / 1000.0
											 : 0.0;
	
	while (CompletedChunkDataQueue.Dequeue(PendingResult))
	{
		if (PendingResult.Chunk.IsValid())
		{
			if (UVoxelChunk* Chunk = PendingResult.Chunk.Get())
			{
				// 플레이어 이동에 따라 LOD가 변경되었으면 Mesh를 재생성하지 않고 넘어감, 다른 queue에 있는 변경된 LOD로 mesh 생성 
				if (PendingResult.Info.LODLevel != Chunk->GetRequestedLODLevel())
				{
					++ProcessedCount;
					++CompletedChunkCount;
					continue;
				}
				
				Chunk->GenerateChunkMesh(PendingResult.Info, MoveTemp(PendingResult.Result));
			}
		}

		++CompletedChunkCount;
		++ProcessedCount;


		// 프레임 드랍 방지 -> MaxChunksPerFrame 만큼의 Chunk만 생성하고 다음 프레임에서 Chunk 생성
		const bool bReachedCountLimit = MaxChunksPerFrame > 0 && ProcessedCount >= MaxChunksPerFrame;
		const bool bReachedTimeLimit = TimeBudgetSeconds > 0.0 && (FPlatformTime::Seconds() - StartTime) >= TimeBudgetSeconds;
		
		if (bReachedCountLimit || bReachedTimeLimit)
			break;
	}

	if (!bLoggedBuildTime && TotalChunkCount > 0 && CompletedChunkCount >= TotalChunkCount)
	{
		const double ElapsedMs = (FPlatformTime::Seconds() - BuildStartTime) * 1000.0;
		UE_LOG(LogTemp, Warning, TEXT("[VoxelManagerComponent] Build Time : %.2f ms"), ElapsedMs);
		bLoggedBuildTime = true;
	}
}

void UVoxelManager::PushCompletedResult(FChunkBuildResult&& Result, const TWeakObjectPtr<UVoxelChunk>& Chunk,
                                        const FChunkSettingInfo& ChunkInfo)
{
	FPendingChunkResult Pending;
	Pending.Chunk = Chunk;
	Pending.Info = ChunkInfo;
	Pending.Result = MoveTemp(Result);

	CompletedChunkDataQueue.Enqueue(Pending);
}

FVector UVoxelManager::GetReferenceLocation() const
{
	if (UWorld* World = GetWorld())
	{
		if (APawn* Pawn = UGameplayStatics::GetPlayerPawn(World, 0))
		{
			return Pawn->GetActorLocation();
		}
	}
	return GetComponentLocation();
}

int32 UVoxelManager::ComputeLODLevel(float Distance) const
{
	int32 LODLevel = 1;

	for (const FLODDistanceLevel& Entry : LODDistanceLevels)
	{
		if (Distance > Entry.DistanceThreshold)
		{
			LODLevel = FMath::Max(LODLevel, Entry.LODLevel);
		}
		else
		{
			break;
		}
	}
	
	return FMath::Clamp(LODLevel, 1, CellNum);
}

void UVoxelManager::UpdateChunkLODLevels(const FVector& ReferenceLocation)
{
	for (auto& Pair : ChunkMap)
	{
		UVoxelChunk* Chunk = Pair.Value;
		if (!IsValid(Chunk))
		{
			continue;
		}

		const float Distance = FVector::Dist(ReferenceLocation, Chunk->GetComponentLocation());
		const int32 DesiredLOD = ComputeLODLevel(Distance);

		if (DesiredLOD == Chunk->GetCurrentLODLevel() || DesiredLOD == Chunk->GetRequestedLODLevel())
		{
			continue;
		}

		FChunkSettingInfo RequestedInfo = Chunk->MakeChunkSettingInfoForLOD(DesiredLOD);
		EnqueueGenerateChunk(Chunk, RequestedInfo);
	}
}

