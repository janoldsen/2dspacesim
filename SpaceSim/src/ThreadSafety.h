#ifndef THREAD_SAFETY_H
#define THREAD_SAFETY_H
#include "atomic.h"


typedef struct
{
	atomic readCount;
	atomic writeCount;
} ReadWriteLock;

typedef struct
{
	atomic count;
} Lock;

// favors writers
void readLock(ReadWriteLock* pLock);
void readRelease(ReadWriteLock* pLock);
void writeLock(ReadWriteLock* pLock);
void writeRelease(ReadWriteLock* pLock);

void lock(Lock* pLock);
void release(Lock* pLock);


#endif