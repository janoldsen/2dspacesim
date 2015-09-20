#ifndef GAME_H
#define GAME_H
#include "GameData.h"

typedef struct BlockInitData
{
	int pos[2];
	ColorMap colorMap;
	DestructionMap destrMap;
} BlockInitData;

typedef struct ShipInitData
{
	ColorPalette colorPalette;
	Matrix transform;
	uint32 numBlocks;
	BlockInitData* blocks;
} ShipInitData;

void addShips(int numShips, ShipInitData* ships);


#endif