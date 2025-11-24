#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "Defines/VoxelStructs.h"
#include "Materials/MaterialInterface.h"
#include "Planet/Planet.h"
#include "VoxelManager.generated.h"

class UVoxelChunk;
class UMaterialInstanceDynamic;

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

	UPROPERTY(VisibleAnywhere, Category="Voxel", meta=(AllowPrivateAccess=true))
	int CellSize;
	UPROPERTY(VisibleAnywhere, Category="Voxel", meta=(AllowPrivateAccess=true))
	int CellNum;
	UPROPERTY(VisibleAnywhere, Category="Voxel", meta=(AllowPrivateAccess=true))
	int ChunkNum;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Voxel", meta=(AllowPrivateAccess=true))
	int PlanetRadius = 0;
	
	void SetPlanetRadius(int InPlanetRadius);
	int32 GetPlanetRadius() const { return PlanetRadius; }
	
	void Sculpt(const FVector& ImpactPoint, float Radius);
	void RecordSculptedDensity(const FChunkSettingInfo& Info, int32 LocalX, int32 LocalY, int32 LocalZ, float Density);
	void ApplySculptedDensityOverrides(const FChunkSettingInfo& Info, TArray<FVertexDensity>& DensityData);

	UFUNCTION(BlueprintCallable, Category="Voxel|Rendering")
	void SetPlanetCenterParameter(const FVector& PlanetCenter);

	void SetVoxelSettings(int32 InCellSize, int32 InCellNum, int32 InChunkNum);
	int32 GetCellSize() const { return CellSize; }
	int32 GetCellNum() const { return CellNum; }
	int32 GetChunkNum() const { return ChunkNum; }
	const FPlanetNoiseSettings& GetNoiseSettings() const { return NoiseSettings; }
	
private:
	UPROPERTY()
	TMap<FIntVector, UVoxelChunk*> ChunkMap;

	UPROPERTY(EditAnywhere, Category="Voxel|Rendering", meta=(AllowPrivateAccess=true))
	TObjectPtr<UMaterialInterface> ChunkMaterial;

	UPROPERTY(Transient)
	TMap<TWeakObjectPtr<UVoxelChunk>, TObjectPtr<UMaterialInstanceDynamic>> ChunkMaterialInstances;
	TObjectPtr<UMaterialInstanceDynamic> ChunkMaterialInstance;
	
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

	UPROPERTY(EditAnywhere, Category="Voxel|Noise", meta=(AllowPrivateAccess=true))
	FPlanetNoiseSettings NoiseSettings;
};
