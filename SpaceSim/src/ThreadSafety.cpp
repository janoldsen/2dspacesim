#include "ThreadSafety.h"
#include "JobSystem.h"


void readLock(ReadWriteLock* pLock)
{
	while (pLock->writeCount > 0)
		jsWait(0);
	atomicIncr(&pLock->readCount);
}

void readRelease(ReadWriteLock* pLock)
{
	atomicDecr(&pLock->readCount);
}

void writeLock(ReadWriteLock* pLock)
{
	while (atomicCmpEx(&pLock->writeCount, 1, 0) != 0)
	{
		jsWait(0);
	}

	while (pLock->readCount > 0)
	{
		jsWait(0);
	}
}

void writeRelease(ReadWriteLock* pLock)
{
	atomicDecr(&pLock->writeCount);
}

void lock(Lock* pLock)
{
	while (atomicCmpEx(&pLock->count, 1, 0) != 0)
	{
		jsWait(0);
	}
}


void release(Lock* pLock)
{
	atomicDecr(&pLock->count);
}