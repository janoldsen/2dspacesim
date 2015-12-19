#ifndef JOB_SYSTEM_H
#define JOB_SYSTEM_H

#include <stdio.h>

#define NUM_FIBERS 160
#define MAIN_THREAD 0

typedef struct JobDecl
{
	void(*fpFunction)(void* pParams);
	void* pParams;
	char* pName;
} JobDecl;


typedef struct Counter Counter;

void initJobSystem();
void shutDownJobSystem();

void startMainThread();

void runJobs(JobDecl* pJobs, int numJobs, Counter** ppCounter);
void runJobsInThread(JobDecl* pJobs, int numJobs, Counter** ppCounter, int threadId);

void waitForCounter(Counter* pCounter);

void printJobSystemDebug(FILE* pFile);

#endif