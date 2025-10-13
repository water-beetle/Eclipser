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

