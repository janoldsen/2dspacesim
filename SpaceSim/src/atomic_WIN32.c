#include "atomic.h"
#include <Windows.h>


void atomicAdd(atomic* pAtomic, int value)
{
	InterlockedAdd(pAtomic, value);
}

void atomicSub(atomic* pAtomic, int value)
{
	InterlockedAdd(pAtomic, -value);
}

void atomicIncr(atomic* pAtomic)
{
	InterlockedIncrement(pAtomic);
}


void atomicDecr(atomic* pAtomic)
{
	InterlockedDecrement(pAtomic);
}


int atomicCmpEx(atomic* pAtomic, int ex, int cmp)
{
	return InterlockedCompareExchange(pAtomic, ex, cmp);
}