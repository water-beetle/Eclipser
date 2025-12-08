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
		bool bModified = false;
	
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
                        				bModified = true;
                        				OwningManager->RecordSculptedDensity(ChunkInfo, x, y, z, CurrentDensity);
                        			}
                        		}
                        }
                }
        }

	if (!bModified)
	{
		return;
	}

	if (OwningManager)
	{
		OwningManager->RequestChunkRebuild(this);
	}
	else
	{
		CachedMeshData = MarchingCubeMeshGenerator::GenerateChunkMesh(ChunkInfo, ChunkDensityData);
		UpdateMesh(CachedMeshData);
	}
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
	const int PlanetRadius = Manager ? Manager->GetPlanetRadius()
			: FMath::Max(0, Info.VoxelSize / 2);
	const int MaxPlanetRadius = FMath::Max(Info.VoxelSize / 2, PlanetRadius);

	const FPlanetNoiseSettings* NoiseSettings = Manager ? &Manager->GetNoiseSettings() : nullptr;
	
	for (int z=0; z < Info.CellNum + 1; z += 1)
	{
		for (int y=0; y < Info.CellNum + 1; y += 1)
		{
			for (int x=0; x < Info.CellNum + 1; x += 1)
			{
				FVector Pos = FVector(x, y, z) * Info.CellSize - FVector(Info.ChunkSize) * 0.5f + Info.ChunkPos;
				OutDensityData[VoxelHelper::GetIndex(x,y,z,Info.CellNum)].Density = CalculateDensity(Pos, PlanetRadius, MaxPlanetRadius, NoiseSettings);
			}
		}
	}

	if (Manager)
	{
		Manager->ApplySculptedDensityOverrides(Info, OutDensityData);
	}
}

float UVoxelChunk::CalculateDensity(const FVector& Pos, int Radius, int MaxRadius, const FPlanetNoiseSettings* NoiseSettings)
{
	const float Distance    = Pos.Size();
	const float BaseRadius  = FMath::Clamp<float>(Radius, 0, MaxRadius);
	const float BaseDensity = BaseRadius - Distance;

	// 노이즈 꺼져 있으면 그냥 구 SDF
	if (!NoiseSettings || !NoiseSettings->bEnableNoise || NoiseSettings->Octaves <= 0)
	{
		return BaseDensity;
	}

	// ─────────────────────────────
	// 2) Warp 적용 (좌표 뒤틀기)
	// ─────────────────────────────
	FVector WarpedPos = Pos;

	const float BaseFrequency = NoiseSettings->BaseFrequency;
	// BaseFrequency가 0 이하면 노이즈 자체가 의미가 없으므로 그대로 반환
	if (BaseFrequency <= 0.0f)
	{
		return BaseDensity;
	}

	if (NoiseSettings->WarpStrength > 0.0f && NoiseSettings->WarpFrequencyMultiplier > 0.0f)
	{
		const float WarpFreq = BaseFrequency * NoiseSettings->WarpFrequencyMultiplier;

		const FVector WarpVector(
			FMath::PerlinNoise3D(Pos * WarpFreq * 1.37f),
			FMath::PerlinNoise3D(Pos * WarpFreq * 0.91f),
			FMath::PerlinNoise3D(Pos * WarpFreq * 1.79f)
		);

		WarpedPos += WarpVector * NoiseSettings->WarpStrength;
	}

	// ─────────────────────────────
	// 3) 옥타브 노이즈 합산
	// ─────────────────────────────
	float Frequency  = BaseFrequency;
	float Amplitude  = NoiseSettings->Amplitude;
	float NoiseValue = 0.0f;

	for (int Octave = 0; Octave < NoiseSettings->Octaves; ++Octave)
	{
		if (Amplitude <= KINDA_SMALL_NUMBER)
		{
			break; // Gain 때문에 너무 작아지면 조기 종료
		}

		const float N = FMath::PerlinNoise3D(WarpedPos * Frequency); // -1 ~ +1
		NoiseValue += N * Amplitude;

		Frequency *= NoiseSettings->Lacunarity;
		Amplitude *= NoiseSettings->Gain;
	}

	// ─────────────────────────────
	// 3-1) 리지드 산맥 노이즈
	// ─────────────────────────────
	if (NoiseSettings->MountainAmplitude > 0.0f && NoiseSettings->MountainFrequency > 0.0f)
	{
		const float MountainFreq = NoiseSettings->MountainFrequency;

		float Ridge = 1.0f - FMath::Abs(FMath::PerlinNoise3D(WarpedPos * MountainFreq));
		Ridge      = FMath::Clamp(Ridge, 0.0f, 1.0f); // 이건 한 번만 Clamp

		Ridge = FMath::Pow(Ridge, NoiseSettings->MountainSharpness);

		// 산맥은 밖으로만 튀어나오게 양수 기여
		NoiseValue += Ridge * NoiseSettings->MountainAmplitude;
	}

	// ─────────────────────────────
	// 4) NoiseValue 크기 제한 (Raise/Depression)
	// ─────────────────────────────
	// Radius + MaxRaise가 MaxRadius를 넘지 않도록 한 번만 제한
	const float MaxPossibleRaise = FMath::Max(0.0f, static_cast<float>(MaxRadius) - BaseRadius);
	const float MaxRaise         = FMath::Min(NoiseSettings->MaxRaise, MaxPossibleRaise);
	const float MaxDepression    = NoiseSettings->MaxDepression;

	NoiseValue = FMath::Clamp(NoiseValue, -MaxDepression, MaxRaise);

	// ─────────────────────────────
	// 5) 최종 반지름 & SDF 계산
	// ─────────────────────────────
	const float SurfaceRadius = FMath::Clamp(BaseRadius + NoiseValue, 0.0f, static_cast<float>(MaxRadius));

	return SurfaceRadius - Distance;
}

