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
		printJobSystemDebug(file);
		Sleep(5);
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


#define BAR(A,B,C) printf(A B C);
#define FOO(...) BAR(__VA_ARGS__)

int main()
{

	FOO("hello", "world", "!\n");
	return 0;


	initJobSystem();

	JobDecl decls0[10];
	JobDecl decls1[10];
	JobDecl decls2[10];
	JobDecl decls[10];

	JobDecl debug;
	debug.fpFunction = debugJobSystem;
	debug.pName = "debug";
	debug.pParams = 0;

	runJobsInThread(&debug, 1, 0, 3);
	
	int numJobs = 1;

	int inputs[10];
	for (int i = 0; i < numJobs; ++i)
	{
		inputs[i] = (i + 1) * 10;

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

	runJobsInThread(decls0, numJobs, &pCounter0, 0);
	runJobsInThread(decls1, numJobs, &pCounter1, 1);
	runJobsInThread(decls2, numJobs, &pCounter2, 2);
	runJobs(decls, numJobs, &pCounter);

	startMainThread();

	waitForCounter(pCounter);
	waitForCounter(pCounter0);
	waitForCounter(pCounter1);
	waitForCounter(pCounter2);

	printf("All done!\n");

	return 0;

	JobDecl job;
	job.fpFunction = mainJob;
	job.pParams = 0;

	runJobs(&job, 1, 0);
	

	return 0;
}