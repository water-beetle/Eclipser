#pragma once
#include "Planet/Voxel/Defines/VoxelStructs.h"

class MarchingCubeMeshGenerator
{
public:
	static FVoxelData GenerateChunkMesh(const FChunkSettingInfo& Info, const TArray<FVertexDensity>& VertexDensityData);

private:
	static FVoxelData GenerateCellMesh(const FChunkSettingInfo& Info, const TArray<FVertexDensity>& VertexDensityData,
	                                   const FIntVector& CellIndex);

	static FVector InterpolateVertex(const FVector& p1, const FVector& p2, float valp1, float valp2);

	static void SetCellCornerIndex(const FIntVector& CellIndex, FVector* V, const FChunkSettingInfo& Info);
};
