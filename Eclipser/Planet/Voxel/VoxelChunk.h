// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "VoxelManager.h"
#include "Components/DynamicMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Defines/VoxelStructs.h"
#include "Planet/Voxel/Defines/VoxelStructs.h"
#include "VoxelChunk.generated.h"


/*
 * 용어 정의
 * Voxel : (ChunkNum,ChunkNum,ChunkNum) 만큼의 Chunk로 이루어진 정육면체
 * 
 * Chunk : (CellNum, CellNum, CellNum) 만큼의 Cell로 이루어진 정육면체
 *
 * Cell : (CellSize, CellSize, CellSize) 크기인 정육면체 -> Planet에서 사용하는 기본 단위 사각형
 */

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ECLIPSER_API UVoxelChunk : public UDynamicMeshComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UVoxelChunk();

	void Build(const FChunkSettingInfo& Info);
	void SetVoxelManager(UVoxelManager* VoxelManager){ OwningManager = VoxelManager; }
	void Sculpt(const FVector_NetQuantize& ImpactPoint);;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;
	virtual void InitializeComponent() override;

public:
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;

private:
	FVoxelData CachedMeshData;
	TArray<FVertexDensity> ChunkDensityData;
	FVoxelDataMappings Mappings;

	void UpdateMesh(const FVoxelData& VoxelMeshData);
	void InitializeChunkDensityData(const FChunkSettingInfo& Info);
	float CalculateDensity(const FVector& Pos, int Radius);

	UPROPERTY()
	UVoxelManager* OwningManager = nullptr;
};
