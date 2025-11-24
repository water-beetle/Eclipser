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

USTRUCT(BlueprintType)
struct FPlanetNoiseSettings
{
	GENERATED_BODY();

	UPROPERTY(EditAnywhere, Category="Noise")
	bool bEnableNoise = true;

	UPROPERTY(EditAnywhere, Category="Noise", meta=(ClampMin=1))
	int32 Octaves = 4;

	UPROPERTY(EditAnywhere, Category="Noise", meta=(ClampMin=0.000001))
	float BaseFrequency = 0.0002f;

	UPROPERTY(EditAnywhere, Category="Noise", meta=(ClampMin=0.0))
	float Amplitude = 300.0f;

	UPROPERTY(EditAnywhere, Category="Noise", meta=(ClampMin=1.0))
	float Lacunarity = 2.0f;

	UPROPERTY(EditAnywhere, Category="Noise", meta=(ClampMin=0.0, ClampMax=1.0))
	float Gain = 0.5f;

	// ─ 거대한 산맥 형태를 위한 리지드 노이즈 ─
	UPROPERTY(EditAnywhere, Category="Noise|Mountain", meta=(ClampMin=0.0))
	float MountainAmplitude = 1200.0f;

	UPROPERTY(EditAnywhere, Category="Noise|Mountain", meta=(ClampMin=0.000001))
	float MountainFrequency = 0.00005f;

	UPROPERTY(EditAnywhere, Category="Noise|Mountain", meta=(ClampMin=0.5f))
	float MountainSharpness = 2.5f;

	// ─ Warp(좌표 뒤틀기) ─
	UPROPERTY(EditAnywhere, Category="Noise|Warp", meta=(ClampMin=0.0))
	float WarpStrength = 30.0f;

	UPROPERTY(EditAnywhere, Category="Noise|Warp", meta=(ClampMin=0.0))
	float WarpFrequencyMultiplier = 2.0f;

	// ─ 안전장치: 노이즈 영향 범위 & 크기 제한 ─
	// Radius 기준으로 "표면 위/아래" 얼마나 깊이까지 노이즈를 줄지
	UPROPERTY(EditAnywhere, Category="Noise|Safety", meta=(ClampMin=1.0))
	float AffectDepth = 800.0f;   // 표면 ±800 범위까지만 노이즈 적용

	// 반지름 기준, 최대 산 높이 / 최대 파이는 깊이
	UPROPERTY(EditAnywhere, Category="Noise|Safety", meta=(ClampMin=0.0))
	float MaxRaise = 800.0f;      // Radius보다 최대 얼마나 더 튀어나갈 수 있는지

	UPROPERTY(EditAnywhere, Category="Noise|Safety", meta=(ClampMin=0.0))
	float MaxDepression = 400.0f; // Radius보다 최대 얼마나 파일 수 있는지
};