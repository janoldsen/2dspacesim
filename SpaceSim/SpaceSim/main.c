#include "JobSystem.h"
#include "Window.h"
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>



void printI(void* i)
{
	for (int j = 0; j < *(int*)i; ++j)
	{
		printf("%d\n", *(int*)i);
	}
}

void mainJob(void* args)
{
	Counter* counter;
	JobDecl jobs[10];
	int argsI[10];
	for (int i = 0; i < 10; ++i)
	{
		argsI[i] = i;
		jobs[i].fpFunction = printI;
		jobs[i].pParams = argsI + i;
	}

	runJobs(jobs, 10, &counter);

	waitForCounter(counter);

	printf("first 10 done now to the next 10\n");

	for (int i = 0; i < 10; ++i)
	{
		argsI[i] = i + 10;
		jobs[i].fpFunction = printI;
		jobs[i].pParams = argsI + i;
	}

	runJobs(jobs, 10, &counter);

	waitForCounter(counter);

	printf("all 20 done\n");

	exit(0);
}


int main()
{
	
	freopen("test.txt", "w", stdout);

	WindowHandle window = createWindow(800, 600);

	initJobSystem();

	JobDecl job;
	job.fpFunction = mainJob;
	job.pParams = 0;

	runJobs(&job, 1, 0);
	startMainThread();


	return 0;
}