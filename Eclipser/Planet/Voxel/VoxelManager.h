#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
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
private:
	UPROPERTY(VisibleAnywhere, meta=(AllowPrivateAccess = true))
	TMap<FIntVector, UVoxelChunk*> ChunkMap;
};
