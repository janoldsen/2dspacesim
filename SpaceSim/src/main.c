#include "JobSystem.h"
#include "FileSystem.h"
#include "Window.h"
#include "Graphics.h"
#include "Time.h"
#include <stdio.h>
#include <stdlib.h>
#include <Windows.h>

// todos
// -check cpu culling vs gpu culling


void mainJob(void* args)
{
	WindowHandle window = createWindow(800, 600);
	initGraphics(800, 600, window);

	Watch watch;
	startWatch(&watch);
	while (1)
	{
		double dt = restartWatch(&watch);

		updateWindow(window);

		render(dt);
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
		uint32 dt = jsWait(1000);
		printf("WAIT_TIME: %d\n", dt);
	}
	fclose(file);
}



int main()
{

	jsInit();
	fsInit("data/", 1000);
	initTime();
	
	JobDecl job;
	job.fpFunction = mainJob;
	job.pParams = 0;
	job.pName = "mainJob";

	jsRunJobsInThread(&job, 1, 0, MAIN_THREAD);
	
	jsStartMainThread();

	return 0;
}