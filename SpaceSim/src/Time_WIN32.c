#include "Time.h"
#include <Windows.h>


uint64 g_freq;

void initTime()
{
	QueryPerformanceFrequency((LARGE_INTEGER*)&g_freq);
}

void startWatch(Watch* pWatch)
{
	QueryPerformanceCounter((LARGE_INTEGER*)&pWatch->start);
}

double stopWatch(Watch* pWatch)
{
	uint64 now;
	QueryPerformanceCounter((LARGE_INTEGER*)&now);

	return (now - pWatch->start) / (double)g_freq;
}

double restartWatch(Watch* pWatch)
{
	uint64 now;
	QueryPerformanceCounter((LARGE_INTEGER*)&now);

	double dt = (now - pWatch->start) / (double)g_freq;
	pWatch->start = now;
	return dt;
}