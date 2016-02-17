#ifndef JOB_SYSTEM_H
#define JOB_SYSTEM_H

#include <stdio.h>
#include "defines.h"

#define NUM_FIBERS 160
#define MAIN_THREAD 0

#define JOB_ENTRY(name) void name(void* pIn)

typedef struct JobDecl
{
	void(*fpFunction)(void* pParams);
	void* pParams;
	char* pName;
} JobDecl;


typedef struct Counter Counter;

void jsInit();
void jsShutDown();

void jsStartMainThread();

void jsRunJobs(JobDecl* pJobs, int numJobs, Counter** ppCounter);
void jsRunJobsInThread(JobDecl* pJobs, int numJobs, Counter** ppCounter, int threadId);
void jsDeleteCounter(Counter* pCounter);
void jsWaitForCounter(Counter* pCounter);
// returns actual waited time
uint32 jsWait(uint32 ms);

void jsPrintDebug(FILE* pFile);

int jsNumThreads();

#endif