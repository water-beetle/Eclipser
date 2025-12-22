#pragma once
#include "VoxelEnums.generated.h"

UENUM(BlueprintType)
enum class ECubeCorner : uint8
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
		 │   (0)──────(4) 
		 │
		 └─────────────────→ Y+ (Right)
*/
	
	BackLeftBottom    UMETA(DisplayName = "Back Left Bottom"),    // (0)
	BackLeftTop       UMETA(DisplayName = "Back Left Top"),       // (1)
	FrontLeftTop      UMETA(DisplayName = "Front Left Top"),      // (2)
	FrontLeftBottom   UMETA(DisplayName = "Front Left Bottom"),   // (3)
	BackRightBottom   UMETA(DisplayName = "Back Right Bottom"),   // (4)
	BackRightTop      UMETA(DisplayName = "Back Right Top"),      // (5)
	FrontRightTop     UMETA(DisplayName = "Front Right Top"),     // (6)
	FrontRightBottom  UMETA(DisplayName = "Front Right Bottom"),  // (7)

	MAX UMETA(Hidden)  // 내부 반복용 (총 8개)
};