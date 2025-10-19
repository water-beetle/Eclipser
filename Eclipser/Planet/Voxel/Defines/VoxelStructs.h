#pragma once

#include "CoreMinimal.h"
#include "VoxelStructs.generated.h"

class UVoxelChunk;

USTRUCT(BlueprintType)
struct FChunkLODConfig
{
	GENERATED_BODY()

	/**
	 * 해당 LOD를 적용하기 시작하는 거리. 
	 * (해당 거리 이상에서 이 설정이 적용되며, 거리는 월드 공간 단위)
	 */
	UPROPERTY(EditAnywhere, Category="Voxel|LOD")
	float Distance = 0.0f;

	/**
	 * LOD 적용 시 사용할 셀 개수 (CellNum). 값이 클수록 높은 디테일.
	 */
	UPROPERTY(EditAnywhere, Category="Voxel|LOD")
	int32 CellNum = 0;
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

	int32 EffectiveCellNum = 0;
	float EffectiveCellSize = 0.0f;
	int32 LODLevel = 0;
	

	void Calculate()
	{
		ChunkSize = CellSize * CellNum;
		VoxelSize = ChunkSize * ChunkNum;

		ChunkPos =  (FVector(ChunkIndex) + 0.5f) * ChunkSize - FVector(VoxelSize * 0.5f);

		const int32 LocalEffectiveCellNum = GetEffectiveCellNum();
		if (LocalEffectiveCellNum > 0)
		{
			EffectiveCellNum = LocalEffectiveCellNum;
			EffectiveCellSize = static_cast<float>(ChunkSize) / static_cast<float>(EffectiveCellNum);
		}
		else
		{
			EffectiveCellNum = CellNum;
			EffectiveCellSize = static_cast<float>(CellSize);
		}
	}

	int32 GetEffectiveCellNum() const
	{
		return EffectiveCellNum > 0 ? EffectiveCellNum : CellNum;
	}

	float GetEffectiveCellSize() const
	{
		return EffectiveCellSize > 0.0f ? EffectiveCellSize : static_cast<float>(CellSize);
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