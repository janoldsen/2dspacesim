#ifndef TIME_H
#define TIME_H

#include "defines.h"

typedef struct
{
	uint64 start;
} Watch;

void initTime();

void startWatch(Watch* pWatch);
double stopWatch(Watch* pWatch);
double restartWatch(Watch* pWatch);


#endif
