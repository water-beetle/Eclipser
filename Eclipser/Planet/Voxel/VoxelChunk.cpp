// Fill out your copyright notice in the Description page of Project Settings.

#include "VoxelChunk.h"

#include "DynamicMesh/MeshNormals.h"
#include "Planet/MarchingCube/MarchingCubeMeshGenerator.h"
#include "Planet/Voxel/etc/VoxelHelper.h"


UVoxelChunk::UVoxelChunk()
{
	PrimaryComponentTick.bCanEverTick = true;

	
}

void UVoxelChunk::Build(const FChunkSettingInfo& Info)
{
	InitializeChunkDensityData(Info);
	CachedMeshData = MarchingCubeMeshGenerator::GenerateChunkMesh(Info, ChunkDensityData);
	UpdateMesh(CachedMeshData);
}

// Called when the game starts
void UVoxelChunk::BeginPlay()
{
	Super::BeginPlay();
}

void UVoxelChunk::InitializeComponent()
{
	Super::InitializeComponent();

	SetComplexAsSimpleCollisionEnabled(true, true);
	bUseAsyncCooking = true; // 멀티스레드로 에셋을 병렬 쿠킹하여 속도를 향상시키는 옵션
	SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetGenerateOverlapEvents(true);
	SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	SetMobility(EComponentMobility::Movable);
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
	if (VoxelMeshData.Triangles.Num() == 0)
	{
		GetDynamicMesh()->Reset();
		SetCollisionEnabled(ECollisionEnabled::NoCollision);
		NotifyMeshUpdated();
		return;
	}

	SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	
	GetDynamicMesh()->EditMesh([&](FDynamicMesh3& EditMesh)
	{
		EditMesh.Clear(); // 필요 시 기존 데이터 초기화
		EditMesh.EnableVertexNormals(FVector3f());

		Mappings.VertexToTriangles.SetNum(VoxelMeshData.Vertices.Num());
		
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
	
	NotifyMeshUpdated();
}

void UVoxelChunk::InitializeChunkDensityData(const FChunkSettingInfo& Info)
{
	ChunkDensityData.SetNum((Info.CellNum+1) * (Info.CellNum+1) * (Info.CellNum+1));
	
	for (int z=0; z < Info.CellNum + 1; z += 1)
	{
		for (int y=0; y < Info.CellNum + 1; y += 1)
		{
			for (int x=0; x < Info.CellNum + 1; x += 1)
			{
				FVector Pos = FVector(x, y, z) * Info.CellSize - FVector(Info.ChunkSize) * 0.5f + Info.ChunkPos;
				ChunkDensityData[VoxelHelper::GetIndex(x,y,z,Info.CellNum)].Density = CalculateDensity(Pos, Info.VoxelSize * 0.3f);
			}
		}
	}
}

float UVoxelChunk::CalculateDensity(const FVector& Pos, int Radius)
{
	float Distance = Pos.Size();
	float Density = Radius - Distance;
	
	//float Noise = FMath::PerlinNoise3D(Position * NoiseScale) * 100.0f;
	return Density;
}

