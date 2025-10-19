#include "MarchingCubeMeshGenerator.h"
#include "MarchingCubeLookupTable.h"
#include "Planet/Voxel/etc/VoxelHelper.h"

FVoxelData MarchingCubeMeshGenerator::GenerateChunkMesh(const FChunkSettingInfo& Info, const TArray<FVertexDensity>& VertexDensityData)
{
	FVoxelData ChunkMeshData;
	
	const int32 EffectiveCellNum = Info.GetEffectiveCellNum();

	for (int z = 0; z < EffectiveCellNum; z += 1)
	{
		for (int y = 0; y < EffectiveCellNum; y += 1)
		{
			for (int x = 0; x < EffectiveCellNum; x += 1)
			{
				FVoxelData CellMeshData = GenerateCellMesh(Info, VertexDensityData, FIntVector(x, y, z));

				int baseIndex = ChunkMeshData.Vertices.Num();
				ChunkMeshData.Vertices.Append(CellMeshData.Vertices);

				for (int triIdx : CellMeshData.Triangles)
					ChunkMeshData.Triangles.Add(baseIndex + triIdx);
			}
		}
	}

	return ChunkMeshData;
}

FVoxelData MarchingCubeMeshGenerator::GenerateCellMesh(const FChunkSettingInfo& Info, const TArray<FVertexDensity>& VertexDensityData,
		const FIntVector& CellIndex)
{
	FVoxelData CellMeshData;
	FVector CellCornerIndex[8]; // Chunk를 기준으로 Cell의 Index 값들
	FVector CellCornerPos[8]; // Cell의 중심을 원점으로 하는 Cell 꼭짓점 좌표들
	float CellCornerDensity[8];
	const int32 EffectiveCellNum = Info.GetEffectiveCellNum();
	const float ChunkSize = static_cast<float>(Info.ChunkSize);
	const float CellSize = Info.GetEffectiveCellSize();

	// Cell의 앞/좌/위 index값을 기준으로 나머지 정육면체 cell의 index 값들을 계산하는 함수
	SetCellCornerIndex(CellIndex, CellCornerIndex, Info);

	// 정육면체 Cell의 8개 Index 값으로, Cell의 중심을 원점으로 하는 꼭짓점 좌표 8개와, Density 값을 배열에 저장
	for (int i = 0; i < 8; i += 1)
	{
		CellCornerDensity[i] = VertexDensityData[VoxelHelper::GetIndex(
						CellCornerIndex[i].X, CellCornerIndex[i].Y, CellCornerIndex[i].Z, EffectiveCellNum)].Density;

		CellCornerPos[i] = CellCornerIndex[i] * CellSize - FVector(ChunkSize) * 0.5f;
	}

	// Marching Cube Algorithm으로 Vertex/Triangle 계산 후 저장
	int cubeIndex = 0;
	for (int i = 0; i < 8; i++)
	{
		if (CellCornerDensity[i] < 0.0f)
			cubeIndex |= (1 << i);
	}

	if (MarchingCubeLooupTable::EdgeTable[cubeIndex] == 0)
		return CellMeshData;

	FVector VertexList[12];
	for (int e = 0; e < 12; ++e)
	{
		if (MarchingCubeLooupTable::EdgeTable[cubeIndex] & (1 << e))
		{
			int c0 = MarchingCubeLooupTable::EdgeVertexIndices[e][0];
			int c1 = MarchingCubeLooupTable::EdgeVertexIndices[e][1];
			VertexList[e] = InterpolateVertex(
				CellCornerPos[c0], CellCornerPos[c1],
				CellCornerDensity[c0], CellCornerDensity[c1]);
		}
	}

	for (int i = 0; MarchingCubeLooupTable::TriTable[cubeIndex][i] != -1; i += 3)
	{
		int idx0 = MarchingCubeLooupTable::TriTable[cubeIndex][i];
		int idx1 = MarchingCubeLooupTable::TriTable[cubeIndex][i + 1];
		int idx2 = MarchingCubeLooupTable::TriTable[cubeIndex][i + 2];

		int vertIndex = CellMeshData.Vertices.Num();
		CellMeshData.Vertices.Add(VertexList[idx0]);
		CellMeshData.Vertices.Add(VertexList[idx1]);
		CellMeshData.Vertices.Add(VertexList[idx2]);

		CellMeshData.Triangles.Add(vertIndex + 2);
		CellMeshData.Triangles.Add(vertIndex + 1);
		CellMeshData.Triangles.Add(vertIndex);
	}
	
	return CellMeshData;
}

FVector MarchingCubeMeshGenerator::InterpolateVertex(const FVector& p1, const FVector& p2, float valp1, float valp2)
{
	float t = (0.0f - valp1) / (valp2 - valp1);
	return FMath::Lerp(p1, p2, t);
}

void MarchingCubeMeshGenerator::SetCellCornerIndex(const FIntVector& CellIndex, FVector* V, const FChunkSettingInfo& Info)
{

/*
		 Z+  (Up)
		 ↑
		 │        (2)──────(6)
		 │       /│        /│
		 │    (1)──────(5)  │
		 │     │ │      │   │
		 │     │(3)-----│--/ (7)
		 │     │/       │ /
		 │   (0)──────(4) ────→ X+ (Forward)
		 │
		 └─────────────────→ Y+ (Right)
*/
	V[4].X = V[5].X = V[0].X = V[1].X = CellIndex.X;
	V[7].X = V[6].X = V[3].X = V[2].X = CellIndex.X + 1;

	V[0].Y = V[1].Y = V[2].Y = V[3].Y = CellIndex.Y;
	V[4].Y = V[5].Y = V[6].Y = V[7].Y = CellIndex.Y + 1;

	V[0].Z = V[3].Z = V[4].Z = V[7].Z = CellIndex.Z;
	V[1].Z = V[2].Z = V[5].Z = V[6].Z = CellIndex.Z + 1;
}
