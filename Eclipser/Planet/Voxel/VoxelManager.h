#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "Defines/VoxelStructs.h"
#include "VoxelManager.generated.h"

class UVoxelChunk;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ECLIPSER_API UVoxelManager : public USceneComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UVoxelManager();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;

	void RegisterChunk(const FIntVector& Index, UVoxelChunk* Chunk);
	UVoxelChunk* GetChunk(const FIntVector& Index);

	UPROPERTY(EditAnywhere, Category="Voxel")
	int CellSize;
	UPROPERTY(EditAnywhere, Category="Voxel")
	int CellNum;
	UPROPERTY(EditAnywhere, Category="Voxel")
	int ChunkNum;

	UPROPERTY(EditAnywhere, Category="Voxel|LOD")
	TArray<FChunkLODConfig> LODSettings;

	void Sculpt(const FVector& ImpactPoint, float Radius);
private:
	UPROPERTY(VisibleAnywhere, meta=(AllowPrivateAccess = true))
	TMap<FIntVector, UVoxelChunk*> ChunkMap;
	
	void EnqueueGenerateChunk(UVoxelChunk* Chunk, const FChunkSettingInfo& ChunkInfo);
	void GenerateCompletedChunk();
	void PushCompletedResult(FChunkBuildResult&& Result, const TWeakObjectPtr<UVoxelChunk>& Chunk, const FChunkSettingInfo& ChunkInfo);
	void SetLOD();
	int32 DetermineLODLevel(float Distance) const;
	int32 DetermineEffectiveCellNum(int32 LODLevel) const;
	void UpdateChunkLODs();
	FVector GetReferenceLocation() const;
private:

	TQueue<FPendingChunkResult, EQueueMode::Mpsc> CompletedChunkDataQueue;
	double BuildStartTime = 0.0;
	int32 TotalChunkCount = 0;
	int32 CompletedChunkCount = 0;
	
	UPROPERTY(EditAnywhere, Category="Voxel|Performance", meta=(ClampMin="0", UIMin="0", AllowPrivateAccess=true))
	int32 MaxChunksPerFrame = 20;
	UPROPERTY(EditAnywhere, Category="Voxel|Performance", meta=(ClampMin="0.0", UIMin="0.0", AllowPrivateAccess=true))
	float ChunkProcessingTimeBudgetMs = 2.0f;
	
	bool bLoggedBuildTime = false;
	TArray<FChunkLODConfig> SortedLODSettings;
	TMap<FIntVector, FChunkSettingInfo> PendingChunkInfos;
};
