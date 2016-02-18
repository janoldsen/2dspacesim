#include "JobSystem.h"
#include "Window.h"
#include "Graphics.h"
#include <stdio.h>
#include <stdlib.h>
#include <Windows.h>

// todos
// -check cpu culling vs gpu culling


void mainJob(void* args)
{
	WindowHandle window = createWindow(800, 600);
	initGraphics(800, 600, window);

	while (1)
	{
		updateWindow(window);

		render();
		present(window);
	}

	exit(0);
}


void debugJobSystem(void* in)
{
	FILE* file = fopen("jobSystem.txt", "w");
	//fclose(file);
	//file = stdout;

	for (;;)
	{
		jsPrintDebug(file);
		uint32 dt = jsWait(100);
		printf("WAIT_TIME: %d\n", dt);
	}
	fclose(file);
}


void print0(void* pIn)
{
	int num = *((int*)pIn);

	for (int i = 0; i < num; ++i)
	{
		printf("thread0: %i(%i)\n", i, num);
		Sleep(10);
	}
}


void print1(void* pIn)
{

	int num = *((int*)pIn);

	for (int i = 0; i < num; ++i)
	{
		printf("thread1: %i(%i)\n", i, num);
		Sleep(10);
	}
}


void print2(void* pIn)
{

	int num = *((int*)pIn);

	for (int i = 0; i < num; ++i)
	{
		printf("thread2: %i(%i)\n", i, num);
		Sleep(10);
	}
}


void print(void* pIn)
{
	int num = *((int*)pIn);

	for (int i = 0; i < num; ++i)
	{
		printf("%i(%i)\n", i, num);
		Sleep(10);
	}
}


int main()
{


	jsInit();

	JobDecl decls0[10];
	JobDecl decls1[10];
	JobDecl decls2[10];
	JobDecl decls[10];

	JobDecl debug;
	debug.fpFunction = debugJobSystem;
	debug.pName = "debug";
	debug.pParams = 0;

	jsRunJobsInThread(&debug, 1, 0, jsNumThreads()-1);
	
	int numJobs = 10;

	#define num 10
	int inputs[num];
	for (int i = 0; i < numJobs; ++i)
	{
		inputs[i] = (i + 1) * num;

		decls[i].fpFunction = print;
		decls[i].pName = "print";
		decls[i].pParams = inputs + i;

		decls0[i].fpFunction = print0;
		decls0[i].pName = "print0";
		decls0[i].pParams = inputs + i;

		decls1[i].fpFunction = print1;
		decls1[i].pName = "print1";
		decls1[i].pParams = inputs + i;

		decls2[i].fpFunction = print2;
		decls2[i].pName = "print2";
		decls2[i].pParams = inputs + i;
	}

	Counter* pCounter0;
	Counter* pCounter1;
	Counter* pCounter2;
	Counter* pCounter;

	
	jsRunJobsInThread(decls0, numJobs, &pCounter0, 0);
	jsRunJobsInThread(decls1, numJobs, &pCounter1, 1);
	jsRunJobsInThread(decls2, numJobs, &pCounter2, 2);
	jsRunJobs(decls, numJobs, &pCounter);

	
	jsStartMainThread();

	jsWaitForCounter(pCounter);
	jsWaitForCounter(pCounter0);
	jsWaitForCounter(pCounter1);
	jsWaitForCounter(pCounter2);

	printf("All done!\n");

	return 0;

	JobDecl job;
	job.fpFunction = mainJob;
	job.pParams = 0;

	jsRunJobs(&job, 1, 0);
	

	return 0;
}