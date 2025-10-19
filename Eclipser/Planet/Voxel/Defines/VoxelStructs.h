#pragma once

#include "CoreMinimal.h"
#include "VoxelStructs.generated.h"

class UVoxelChunk;

USTRUCT(BlueprintType)
struct FLODDistanceLevel
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category="Voxel|LOD", meta=(ClampMin="1", UIMin="1"))
	int32 LODLevel = 1;

	UPROPERTY(EditAnywhere, Category="Voxel|LOD", meta=(ClampMin="0.0", UIMin="0.0"))
	float DistanceThreshold = 0.0f;
};

struct FChunkSettingInfo
{/*
 * 용어 정의
 * Voxel : (ChunkNum,ChunkNum,ChunkNum) 만큼의 Chunk로 이루어진 정육면체
 * 
 * Chunk : (CellNum, CellNum, CellNum) 만큼의 Cell로 이루어진 정육면체
 *
 * Cell : (CellSize, CellSize, CellSize) 크기인 정육면체 -> Planet에서 사용하는 기본 단위 사각형
 *
 * ChunkIndex : Voxel에의 Chunk 위치 -> (x,y,z) 위치를 1차원 배열 index 값으로 변경한 값 
 */
	FIntVector ChunkIndex;
	int CellSize;
	int CellNum;
	int ChunkNum;
	int LODLevel = 1;

	
	int ChunkSize;
	int VoxelSize;
	
	FVector ChunkPos; // World Space 기준 현재 Chunk의 중심 좌표

	void Calculate()
	{
		ChunkSize = CellSize * CellNum;
		VoxelSize = ChunkSize * ChunkNum;

		ChunkPos =  (FVector(ChunkIndex) + 0.5f) * ChunkSize - FVector(VoxelSize * 0.5f);
	}
};

struct FVoxelData
{
	TArray<FVector> Vertices;
	TArray<FVector> Normals;
	TArray<FColor> Colors;
	TArray<int> Triangles;
};

struct FVertexDensity
{
	FVertexDensity()
		: Density(0.0f), Id(0) {};
	
	FVertexDensity(const float Density, const int Id)
	{
		this->Density = Density;
		this->Id = Id;
	};
    
	float Density;
	int Id;
};

struct FVoxelDataMappings
{
	TArray<TSet<int32>> VertexToTriangles;
	TMap<FIntVector, TSet<int32>> CellToTriangles;
	TMap<FIntVector, TArray<int32>> CellToVertices;
	bool bIsLoaded = false;
};

struct FChunkBuildResult
{
	FVoxelData MeshData;
	TArray<FVertexDensity> DensityData;
};

struct FPendingChunkResult
{
	TWeakObjectPtr<UVoxelChunk> Chunk;
	FChunkSettingInfo Info;
	FChunkBuildResult Result;
};

struct FChunkGenerationRequest
{
	UVoxelChunk* Chunk = nullptr;
	FChunkSettingInfo Info;
	float DistanceSquared = 0.0f;
};