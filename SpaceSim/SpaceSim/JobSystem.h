#ifndef JOB_SYSTEM_H
#define JOB_SYSTEM_H


#define NUM_FIBERS 160


struct JobDecl
{
	void(*fpFunction)(void* pParams);
	void* pParams;
} typedef JobDecl;


typedef struct Counter Counter;

void initJobSystem();
void shutDownJobSystem();

void startMainThread();

void runJobs(JobDecl* jobs, int numJobs, struct Counter** counter);

void waitForCounter(Counter* pCounter);

#endif