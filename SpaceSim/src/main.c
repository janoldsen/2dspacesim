#include "JobSystem.h"
#include "Window.h"
#include "Graphics.h"
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>


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


int main()
{
	
	
	initJobSystem();

	JobDecl job;
	job.fpFunction = mainJob;
	job.pParams = 0;

	runJobs(&job, 1, 0);
	startMainThread();


	return 0;
}