// Fill out your copyright notice in the Description page of Project Settings.

#include "VoxelManager.h"
#include "Planet/Voxel/VoxelChunk.h"
#include "Defines/VoxelStructs.h"


// Sets default values for this component's properties
UVoxelManager::UVoxelManager()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UVoxelManager::BeginPlay()
{
	Super::BeginPlay();
	check(GetOwner());

	const double StartTime = FPlatformTime::Seconds();

	// Voxel은 Actor의 Location을 중점으로 생성됨
	for (int32 x = 0; x < ChunkNum; ++x)
		for (int32 y = 0; y < ChunkNum; ++y)
			for (int32 z = 0; z < ChunkNum; ++z)
			{
				FChunkSettingInfo ChunkInfo{ FIntVector(x,y,z), CellSize, CellNum, ChunkNum};
				ChunkInfo.Calculate();
				
				UVoxelChunk* Chunk = NewObject<UVoxelChunk>(GetOwner());
				Chunk->RegisterComponent();
				Chunk->AttachToComponent(this, FAttachmentTransformRules::KeepRelativeTransform);
				Chunk->SetRelativeLocation(ChunkInfo.ChunkPos);
				
				Chunk->Build(ChunkInfo);
				Chunk->SetVoxelManager(this);

				RegisterChunk(FIntVector(x,y,z), Chunk);
			}

	const double ElapsedMs = (FPlatformTime::Seconds() - StartTime) * 1000.0;
	UE_LOG(LogTemp, Warning, TEXT("[VoxelManagerComponent] Build Time : %.2f ms"), ElapsedMs);
	
}


// Called every frame
void UVoxelManager::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
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

