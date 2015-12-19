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
	fclose(file);
	file = stdout;

	for (;;)
	{
		printJobSystemDebug(file);
		//Sleep(100);
	}
}


void isDivideable(void* in)
{
	struct { int i; int min; int max; int out; } *input = in;
	
	for (int i = input->min; i < input->max; i++)
	{
		if ((input->i % i) == 0)
		{
			//printf("%i is divideable by %i!/n", input->i, i);
			input->out = 1;
			return;
		}
			
	}

	

}


#define NUM_NEW_JOBS 16
void isPrime(void* in)
{
	int i = *(int*)in;
	struct { int i; int min; int max; int out; } inputs[128];
	JobDecl decl[NUM_NEW_JOBS];
	Counter* pCounter;

	for (int i = 0; i < NUM_NEW_JOBS; ++i)
	{
		inputs[i].i = i;
		inputs[i].min = i / NUM_NEW_JOBS * i;
		inputs[i].max = i / NUM_NEW_JOBS * (i + 1);

		decl[i].fpFunction = isDivideable;
		decl[i].pName = "isDiviedable";
		decl[i].pParams = inputs + i;
	}

	runJobs(decl, NUM_NEW_JOBS, &pCounter);

	waitForCounter(pCounter);

	for (int i = 0; i < NUM_NEW_JOBS; ++i)
	{
		if (inputs[i].out != 0)
		{
			printf("%i is not prime!\n", i);
			return;
		}
	}
	printf("%i is prime!\n", i);
}


int main()
{
	initJobSystem();

	JobDecl debugJob;
	debugJob.fpFunction = debugJobSystem;
	debugJob.pName = "debug";
	debugJob.pParams = NULL;

	runJobsInThread(&debugJob, 1, NULL, 3);


	JobDecl primeJobs[32];
	int numbers[32];
	Counter* pCounter;

	for (int i = 0; i < 32; ++i)
	{
		numbers[i] = i + 123456;
		primeJobs[i].fpFunction = isPrime;
		primeJobs[i].pParams = numbers + i;
		primeJobs[i].pName = "isPrime";
	}

	runJobsInThread(primeJobs, 32, &pCounter, MAIN_THREAD);

	startMainThread();

	waitForCounter(pCounter);

	JobDecl job;
	job.fpFunction = mainJob;
	job.pParams = 0;

	runJobs(&job, 1, 0);
	

	return 0;
}