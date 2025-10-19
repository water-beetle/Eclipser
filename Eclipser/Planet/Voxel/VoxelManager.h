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

	void Sculpt(const FVector& ImpactPoint, float Radius);
	void RecordSculptedDensity(const FChunkSettingInfo& Info, int32 LocalX, int32 LocalY, int32 LocalZ, float Density);
	void ApplySculptedDensityOverrides(const FChunkSettingInfo& Info, TArray<FVertexDensity>& DensityData);
	
private:
	UPROPERTY(VisibleAnywhere, meta=(AllowPrivateAccess = true))
	TMap<FIntVector, UVoxelChunk*> ChunkMap;
	
	void GenerateChunk();
	void EnqueueGenerateChunk(UVoxelChunk* Chunk, const FChunkSettingInfo& ChunkInfo);
	void GenerateCompletedChunk();
	void PushCompletedResult(FChunkBuildResult&& Result, const TWeakObjectPtr<UVoxelChunk>& Chunk, const FChunkSettingInfo& ChunkInfo);

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

private:
	/* LOD Settings */
	int32 ComputeLODLevel(float Distance) const;
	void UpdateChunkLODLevels(const FVector& Vector);

	float TimeSinceLastLODUpdate = 0.0f;
	
	UPROPERTY(EditAnywhere, Category="Voxel|LOD", meta=(AllowPrivateAccess=true))
	TArray<FLODDistanceLevel> LODDistanceLevels;
	UPROPERTY(EditAnywhere, Category="Voxel|LOD", meta=(ClampMin="0.0", UIMin="0.0", AllowPrivateAccess=true))
	float LODUpdateInterval = 0.2f;

private:
	/* Sculpt Settings*/
	mutable FCriticalSection SculptedDensityLock;
	struct FChunkSculptOverrides
	{
		TMap<int32, FFloat16> VertexDensities;
	};
	TMap<FIntVector, FChunkSculptOverrides> SculptedDensityMap;
};
