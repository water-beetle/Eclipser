#include "VoxelHelper.h"

int32 VoxelHelper::GetIndex(int X, int Y, int Z, int CellNum)
{
	return X + Y * (CellNum + 1) + Z * (CellNum + 1) * (CellNum + 1);
}
