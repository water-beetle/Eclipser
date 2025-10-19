// Fill out your copyright notice in the Description page of Project Settings.

#include "VoxelManager.h"
#include "Planet/Voxel/VoxelChunk.h"
#include "Defines/VoxelStructs.h"
#include "Kismet/GameplayStatics.h"


// Sets default values for this component's properties
UVoxelManager::UVoxelManager()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


void UVoxelManager::SetLOD()
{
	// LOD 거리 순으로 정렬
	SortedLODSettings = LODSettings;
	SortedLODSettings.Sort([](const FChunkLODConfig& A, const FChunkLODConfig& B)
	{
		return A.Distance < B.Distance;
	});

	// Distance가 0인 항목이 반드시 추가 되어 있어야 함
	const bool bHasBaseLOD = SortedLODSettings.ContainsByPredicate([](const FChunkLODConfig& Config)
	{
		return Config.Distance <= KINDA_SMALL_NUMBER;
	});

	if (!bHasBaseLOD)
	{
		FChunkLODConfig BaseConfig;
		BaseConfig.Distance = 0.0f;
		BaseConfig.CellNum = CellNum;
		SortedLODSettings.Insert(BaseConfig, 0);
	}

	if (SortedLODSettings.Num() > 0)
	{
		SortedLODSettings[0].Distance = 0.0f;
		if (SortedLODSettings[0].CellNum <= 0)
		{
			SortedLODSettings[0].CellNum = CellNum;
		}
		SortedLODSettings[0].CellNum = FMath::Clamp(SortedLODSettings[0].CellNum, 1, CellNum);
	}
	
	for (int32 Index = 1; Index < SortedLODSettings.Num(); ++Index)
	{
		FChunkLODConfig& Config = SortedLODSettings[Index];
		const FChunkLODConfig& PrevConfig = SortedLODSettings[Index - 1];

		Config.Distance = FMath::Max(Config.Distance, PrevConfig.Distance);
		if (Config.CellNum <= 0)
		{
			Config.CellNum = FMath::Max(1, PrevConfig.CellNum / 2);
		}
		Config.CellNum = FMath::Clamp(Config.CellNum, 1, CellNum);
	}
}

int32 UVoxelManager::DetermineLODLevel(float Distance) const
{
	if (SortedLODSettings.Num() == 0)
	{
		return 0;
	}

	int32 SelectedIndex = 0;
	for (int32 Index = 0; Index < SortedLODSettings.Num(); ++Index)
	{
		if (Distance >= SortedLODSettings[Index].Distance)
		{
			SelectedIndex = Index;
		}
		else
		{
			break;
		}
	}

	return SelectedIndex;
}

int32 UVoxelManager::DetermineEffectiveCellNum(int32 LODLevel) const
{
	if (SortedLODSettings.Num() == 0)
	{
		return CellNum;
	}

	const int32 Index = FMath::Clamp(LODLevel, 0, SortedLODSettings.Num() - 1);
	const int32 ConfigCellNum = SortedLODSettings[Index].CellNum;
	return FMath::Clamp(ConfigCellNum > 0 ? ConfigCellNum : CellNum, 1, CellNum);
}

void UVoxelManager::UpdateChunkLODs()
{
	if (ChunkMap.Num() == 0)
	{
		return;
	}

	const FVector ReferenceLocation = GetReferenceLocation();

	for (const TPair<FIntVector, UVoxelChunk*>& Pair : ChunkMap)
	{
		UVoxelChunk* Chunk = Pair.Value;
		if (!IsValid(Chunk))
		{
			continue;
		}

		const float Distance = FVector::Dist(ReferenceLocation, Chunk->GetComponentLocation());
		const int32 DesiredLOD = DetermineLODLevel(Distance);

		const FChunkSettingInfo* PendingInfo = PendingChunkInfos.Find(Pair.Key);
		const int32 CurrentLOD = PendingInfo ? PendingInfo->LODLevel : Chunk->GetChunkInfo().LODLevel;

		if (DesiredLOD == CurrentLOD)
		{
			continue;
		}

		if (PendingInfo != nullptr)
		{
			continue;
		}

		FChunkSettingInfo NewInfo = Chunk->GetChunkInfo();
		NewInfo.LODLevel = DesiredLOD;
		NewInfo.EffectiveCellNum = DetermineEffectiveCellNum(DesiredLOD);
		NewInfo.Calculate();

		EnqueueGenerateChunk(Chunk, NewInfo);
	}
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

// Called when the game starts
void UVoxelManager::BeginPlay()
{
	Super::BeginPlay();
	check(GetOwner());

	if (ChunkNum <= 0 || CellNum <= 0 || CellSize <= 0)
	{
		return;
	}

	SetLOD();

	TArray<FChunkGenerationRequest> GenerationRequests;
	GenerationRequests.Reserve(ChunkNum * ChunkNum * ChunkNum);

	const FVector ReferenceLocation = GetReferenceLocation();
	
	// Voxel은 Actor의 Location을 중점으로 생성됨
	for (int32 x = 0; x < ChunkNum; ++x)
		for (int32 y = 0; y < ChunkNum; ++y)
			for (int32 z = 0; z < ChunkNum; ++z)
			{
				FChunkSettingInfo ChunkInfo{ FIntVector(x,y,z), CellSize, CellNum, ChunkNum};
				ChunkInfo.Calculate();

				const FVector ChunkWorldLocation = GetComponentTransform().TransformPosition(ChunkInfo.ChunkPos);
				const float DistanceSquared = FVector::DistSquared(ReferenceLocation, ChunkWorldLocation);
				const float Distance = FMath::Sqrt(DistanceSquared);

				const int32 LODLevel = DetermineLODLevel(Distance);
				ChunkInfo.LODLevel = LODLevel;
				ChunkInfo.EffectiveCellNum = DetermineEffectiveCellNum(LODLevel);
				ChunkInfo.Calculate();
				
				UVoxelChunk* Chunk = NewObject<UVoxelChunk>(GetOwner());
				Chunk->RegisterComponent();
				Chunk->AttachToComponent(this, FAttachmentTransformRules::KeepRelativeTransform);
				Chunk->SetRelativeLocation(ChunkInfo.ChunkPos);
				
				Chunk->SetVoxelManager(this);

				RegisterChunk(FIntVector(x,y,z), Chunk);

				FChunkGenerationRequest& Request = GenerationRequests.Emplace_GetRef();
				Request.Chunk = Chunk;
				Request.Info = ChunkInfo;
				Request.DistanceSquared = DistanceSquared;
			}

	GenerationRequests.Shrink();
	Algo::SortBy(GenerationRequests, &FChunkGenerationRequest::DistanceSquared);

	for (FChunkGenerationRequest& Request : GenerationRequests)
	{
		EnqueueGenerateChunk(Request.Chunk, Request.Info);
		++TotalChunkCount;
	}
}


// Called every frame
void UVoxelManager::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	GenerateCompletedChunk();
	UpdateChunkLODs();
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

void UVoxelManager::EnqueueGenerateChunk(UVoxelChunk* Chunk, const FChunkSettingInfo& ChunkInfo)
{
	if (!IsValid(Chunk)) return;

	FChunkSettingInfo& PendingInfo = PendingChunkInfos.FindOrAdd(ChunkInfo.ChunkIndex);
	PendingInfo = ChunkInfo;
	
	TWeakObjectPtr<UVoxelManager> ManagerPtr(this);
	TWeakObjectPtr<UVoxelChunk> ChunkPtr(Chunk);

	UE::Tasks::Launch(
		UE_SOURCE_LOCATION, [ManagerPtr, ChunkPtr, ChunkInfo]()
		{
			FChunkBuildResult Result = UVoxelChunk::GenerateChunkData(ChunkInfo);

		   if (UVoxelManager* Manager = ManagerPtr.Get())
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
				Chunk->GenerateChunkMesh(PendingResult.Info, MoveTemp(PendingResult.Result));
			}
		}

		PendingChunkInfos.Remove(PendingResult.Info.ChunkIndex);

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

