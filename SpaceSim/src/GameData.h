#ifndef GAME_DATA_H
#define GAME_DATA_H
#include "defines.h"
#include "Math.h"
#define NUM_STAGES 3


typedef struct PhysicsState PhysicsState;
typedef struct PhysicsShipData PhysicsShipData;

typedef struct DestructionMap
{
	uint32 a[2]
} DestructionMap;


typedef struct ColorMap
{
	uint32 a[6];
} ColorMap;

typedef struct
{
	uint32 colors[8];
} ColorPalette;

typedef struct 
{
	Matrix* transforms;
	ColorPalette* colorPalettes;
	PhysicsShipData* physicsData;
} ShipData;

typedef struct
{
	DestructionMap* destructionMaps;
	ColorMap* colorMaps;
} BlockData;

typedef struct FrameData
{
	float dt;
	 
	ShipData shipData;
	BlockData blockData;

	PhysicsState* physicsState;
} FrameData;




#endif
