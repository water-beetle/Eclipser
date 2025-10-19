// Fill out your copyright notice in the Description page of Project Settings.

#include "VoxelChunk.h"

#include "DynamicMesh/MeshNormals.h"
#include "Planet/MarchingCube/MarchingCubeMeshGenerator.h"
#include "Planet/Voxel/etc/VoxelHelper.h"


UVoxelChunk::UVoxelChunk()
{
	PrimaryComponentTick.bCanEverTick = true;
	
}

void UVoxelChunk::GenerateChunkMesh(const FChunkSettingInfo& Info, FChunkBuildResult&& Result)
{
	ChunkInfo = Info;
	ChunkDensityData = MoveTemp(Result.DensityData);
	CachedMeshData = MoveTemp(Result.MeshData);
	CurrentLODLevel = Info.LODLevel;
	RequestedLODLevel = Info.LODLevel;
	UpdateMesh(CachedMeshData);
}

FChunkBuildResult UVoxelChunk::GenerateChunkData(const FChunkSettingInfo& Info, UVoxelManager* Manager)
{
	// 단순 계산이라 스레드 처리 가능
	FChunkBuildResult Result;
	GenerateChunkDensityData(Info, Result.DensityData, Manager);
	Result.MeshData = MarchingCubeMeshGenerator::GenerateChunkMesh(Info, Result.DensityData);
	return Result;
}

void UVoxelChunk::InitializeChunk(const FChunkSettingInfo& Info)
{
	ChunkInfo = Info;
	ChunkInfo.LODLevel = FMath::Max(1, ChunkInfo.LODLevel);
	ChunkInfo.Calculate();

	CurrentLODLevel = ChunkInfo.LODLevel;
	RequestedLODLevel = ChunkInfo.LODLevel;
}

FChunkSettingInfo UVoxelChunk::MakeChunkSettingInfoForLOD(int32 LODLevel) const
{
	FChunkSettingInfo Info = ChunkInfo;
	Info.LODLevel = FMath::Max(1, LODLevel);
	Info.Calculate();
	return Info;
}

void UVoxelChunk::SetRequestedLODLevel(int InLODLevel)
{
	RequestedLODLevel = FMath::Max(1, InLODLevel);
}

void UVoxelChunk::Sculpt(const FVector& ImpactPoint, float Radius)
{
	if (ChunkInfo.CellNum <= 0 || ChunkInfo.CellSize <= 0)
                return;

        const FVector ChunkCenter = GetComponentLocation();
        const FVector ChunkExtent = FVector(ChunkInfo.ChunkSize) * 0.5f;
        const FVector ChunkMin = ChunkCenter - ChunkExtent;
        const FVector ChunkMax = ChunkCenter + ChunkExtent;

        const FVector SphereMin = FVector(ImpactPoint) - FVector(Radius);
        const FVector SphereMax = FVector(ImpactPoint) + FVector(Radius);

        if (SphereMax.X < ChunkMin.X || SphereMin.X > ChunkMax.X ||
            SphereMax.Y < ChunkMin.Y || SphereMin.Y > ChunkMax.Y ||
            SphereMax.Z < ChunkMin.Z || SphereMin.Z > ChunkMax.Z)
        {
                return;
        }

        const float CellSize = static_cast<float>(ChunkInfo.CellSize);

        auto ToMinIndex = [&](float Value, float MinBound) -> int32
        {
                const float Normalized = (Value - MinBound) / CellSize;
                return FMath::Clamp(FMath::FloorToInt(Normalized), 0, ChunkInfo.CellNum);
        };

        auto ToMaxIndex = [&](float Value, float MinBound) -> int32
        {
                const float Normalized = (Value - MinBound) / CellSize;
                return FMath::Clamp(FMath::CeilToInt(Normalized), 0, ChunkInfo.CellNum);
        };

        const int32 StartX = ToMinIndex(FMath::Max(SphereMin.X, ChunkMin.X), ChunkMin.X);
        const int32 StartY = ToMinIndex(FMath::Max(SphereMin.Y, ChunkMin.Y), ChunkMin.Y);
        const int32 StartZ = ToMinIndex(FMath::Max(SphereMin.Z, ChunkMin.Z), ChunkMin.Z);

        const int32 EndX = ToMaxIndex(FMath::Min(SphereMax.X, ChunkMax.X), ChunkMin.X);
        const int32 EndY = ToMaxIndex(FMath::Min(SphereMax.Y, ChunkMax.Y), ChunkMin.Y);
        const int32 EndZ = ToMaxIndex(FMath::Min(SphereMax.Z, ChunkMax.Z), ChunkMin.Z);

        if (StartX > EndX || StartY > EndY || StartZ > EndZ)
                return;

        const FVector SphereCenter(ImpactPoint);
        const float RadiusSquared = Radius * Radius;

        for (int32 z = StartZ; z <= EndZ; ++z)
        {
                for (int32 y = StartY; y <= EndY; ++y)
                {
                        for (int32 x = StartX; x <= EndX; ++x)
                        {
                                const int32 VertexIndex = VoxelHelper::GetIndex(x, y, z, ChunkInfo.CellNum);
                                FVector VertexPosition = ChunkMin + FVector(x, y, z) * CellSize;

                                const float DistanceSquared = FVector::DistSquared(VertexPosition, SphereCenter);
                                if (DistanceSquared > RadiusSquared)
                                        continue;

                                const float Distance = FMath::Sqrt(DistanceSquared);
                                const float TargetDensity = Distance - Radius;
                        		float& CurrentDensity = ChunkDensityData[VertexIndex].Density;
                        		const float NewDensity = FMath::Min(CurrentDensity, TargetDensity);
                        		if (!FMath::IsNearlyEqual(CurrentDensity, NewDensity))
                        		{
                        			CurrentDensity = NewDensity;
                        			if (OwningManager)
                        			{
                        				OwningManager->RecordSculptedDensity(ChunkInfo, x, y, z, CurrentDensity);
                        			}
                        		}
                        }
                }
        }

        CachedMeshData = MarchingCubeMeshGenerator::GenerateChunkMesh(ChunkInfo, ChunkDensityData);
        UpdateMesh(CachedMeshData);
}

// Called when the game starts
void UVoxelChunk::BeginPlay()
{
	Super::BeginPlay();
}

void UVoxelChunk::OnRegister()
{
	Super::OnRegister();

	SetCollisionProfileName(TEXT("Dig"));
	SetComplexAsSimpleCollisionEnabled(true, true);
	bUseAsyncCooking = true;
	SetMobility(EComponentMobility::Movable);
	SetGenerateOverlapEvents(true);
}

// Called every frame
void UVoxelChunk::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

void UVoxelChunk::UpdateMesh(const FVoxelData& VoxelMeshData)
{
	// 삼각형 데이터가 없으면 메시와 충돌을 초기화한 뒤 종료
	if (VoxelMeshData.Vertices.Num() == 0 || VoxelMeshData.Triangles.Num() == 0)
	{
		GetDynamicMesh()->Reset();
		//SetCollisionEnabled(ECollisionEnabled::NoCollision);
		NotifyMeshUpdated();
		return;
	}
	
	GetDynamicMesh()->EditMesh([&](FDynamicMesh3& EditMesh)
	{
		EditMesh.Clear();
		EditMesh.EnableVertexNormals(FVector3f());

		//Mappings.VertexToTriangles.SetNum(VoxelMeshData.Vertices.Num());
		
		TArray<int32> VIDs;
		VIDs.Reserve(VoxelMeshData.Vertices.Num());

		// 정점 추가
		for (int i = 0; i < VoxelMeshData.Vertices.Num(); i++)
		{
			int32 ID = EditMesh.AppendVertex(VoxelMeshData.Vertices[i]);
			VIDs.Add(ID);
		}

		// 삼각형 추가
		for (int i = 0; i < VoxelMeshData.Triangles.Num(); i += 3)
		{
			int32 T0 = VIDs[VoxelMeshData.Triangles[i]];
			int32 T1 = VIDs[VoxelMeshData.Triangles[i + 1]];
			int32 T2 = VIDs[VoxelMeshData.Triangles[i + 2]];

			int32 TriID = EditMesh.AppendTriangle(T0, T1, T2);

			// Mappings.VertexToTriangles[T0].Add(TriID);
			// Mappings.VertexToTriangles[T1].Add(TriID);
			// Mappings.VertexToTriangles[T2].Add(TriID);
			//
			// FIntVector Cell = GetCellFromTriangle(EditMesh, T0, T1, T2);
			// Mappings.CellToTriangles.FindOrAdd(Cell).Add(TriID);
			// Mappings.CellToVertices.FindOrAdd(Cell).AddUnique(T0);
			// Mappings.CellToVertices[Cell].AddUnique(T1);
			// Mappings.CellToVertices[Cell].AddUnique(T2);
		}

		// 노멀 재계산
		UE::Geometry::FMeshNormals::QuickComputeVertexNormals(EditMesh);
	});
	
	//SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	NotifyMeshUpdated();
}

void UVoxelChunk::GenerateChunkDensityData(const FChunkSettingInfo& Info, TArray<FVertexDensity>& OutDensityData, UVoxelManager* Manager)
{
	OutDensityData.SetNum((Info.CellNum+1) * (Info.CellNum+1) * (Info.CellNum+1));
	
	for (int z=0; z < Info.CellNum + 1; z += 1)
	{
		for (int y=0; y < Info.CellNum + 1; y += 1)
		{
			for (int x=0; x < Info.CellNum + 1; x += 1)
			{
				FVector Pos = FVector(x, y, z) * Info.CellSize - FVector(Info.ChunkSize) * 0.5f + Info.ChunkPos;
				OutDensityData[VoxelHelper::GetIndex(x,y,z,Info.CellNum)].Density = CalculateDensity(Pos, Info.VoxelSize * 0.3f);
			}
		}
	}

	if (Manager)
	{
		Manager->ApplySculptedDensityOverrides(Info, OutDensityData);
	}
}

float UVoxelChunk::CalculateDensity(const FVector& Pos, int Radius)
{
	float Distance = Pos.Size();
	float Density = Radius - Distance;
	
	//float Noise = FMath::PerlinNoise3D(Position * NoiseScale) * 100.0f;
	return Density;
}

