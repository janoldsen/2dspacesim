#ifdef _WIN32
#include "JobSystem.h"
#include <Windows.h>
#include <assert.h>

#define MAX_NUMBER_PROCESSORS 16
#define FIBER_STACK_SIZE (1 << 16)
#define MAX_GLOBAL_JOB_QUEUE_SIZE 1024
#define MAX_LOCAL_JOB_QUEUE_SIZE 256
#define MAX_COUNTERS 1024


// no seperate queues for local jobs,
// store thread in fiber as bitpattern
// and check thread before popping...

typedef long atomic;

typedef struct
{
	void(*fpFunction)(void* pParams);
	void* pParams;
	char* pName;
	Counter* pCounter;
} Job;



enum
{
	RUNNING,
	NO_JOB,
	WAITING,
	FREE,
	UNUSED,
} typedef FiberStatus;

typedef struct Fiber
{
	LPVOID fiber;
	struct Fiber* pNextInWaitList;
	FiberStatus status;
	int currentThread;
	Job* currentJob;
} Fiber;


typedef struct Counter
{
	Fiber* pWaitList;
	atomic counter;
	atomic lock;
} Counter;

typedef struct
{
	atomic front;
	atomic back;
	atomic backRdy;
	Job* pJobs;
	int maxJobs;
} JobQueue;


static atomic g_fiberPoolHead = NUM_FIBERS - 1;
static Fiber* g_pFiberPool[NUM_FIBERS];
static Fiber g_fibers[NUM_FIBERS];

static int g_numProcessors;
static HANDLE g_threads[MAX_NUMBER_PROCESSORS];


static JobQueue g_globalJobQueue;
static Job g_globalJobs[MAX_GLOBAL_JOB_QUEUE_SIZE];

static JobQueue g_localJobQueues[MAX_NUMBER_PROCESSORS];
static Job g_localJobs[MAX_LOCAL_JOB_QUEUE_SIZE][MAX_NUMBER_PROCESSORS];

static atomic g_counterLock = 0;
static Counter* g_pNextCounter;
static Counter g_counters[MAX_COUNTERS];

static atomic g_globalWaitListLock;
static Fiber* g_pGlobalWaitList;

static int g_threadIds[MAX_NUMBER_PROCESSORS];

static void allocCounter(Counter** ppCounter);

static void pushJobs(JobDecl* pJobDecls, int numJobs, Counter** ppCounter, JobQueue* pQueue)
{
	// grab memory space
	for (int i = 0; i < numJobs; ++i)
	{
		int oldBack;
		// get back
		for (;;)
		{
			oldBack = pQueue->back;

			int newBack = (oldBack + 1) % pQueue->maxJobs;

			if (InterlockedCompareExchange(&pQueue->back, newBack, oldBack) == oldBack)
				break;
		}

		//alloc counter
		if (ppCounter != NULL)
		{
			allocCounter(ppCounter);
			(*ppCounter)->counter = numJobs;
		}


		pQueue->pJobs[oldBack].fpFunction = pJobDecls[i].fpFunction;
		pQueue->pJobs[oldBack].pParams = pJobDecls[i].pParams;
		pQueue->pJobs[oldBack].pName = pJobDecls[i].pName;
		pQueue->pJobs[oldBack].pCounter = ppCounter == 0 ? 0 : *ppCounter;


		// ready back
		for (;;)
		{
			if (InterlockedCompareExchange(&pQueue->backRdy, oldBack+1, oldBack) == oldBack)
				break;
		}
	}
}

static Job* popJob(JobQueue* pQueue)
{

	
	for (;;)
	{
		// check if empty
		if (pQueue->front == pQueue->backRdy)
			return NULL;

		int oldFront = pQueue->front;
		int newFront = (oldFront == MAX_GLOBAL_JOB_QUEUE_SIZE - 1) ? 0 : oldFront + 1;
		if (InterlockedCompareExchange(&pQueue->front, newFront, oldFront) == oldFront)
			return &pQueue->pJobs[oldFront];
	}

}

void initCounterAllocator()
{
	Counter* pIter = g_counters + MAX_COUNTERS;
	Counter* pPrev = NULL;
	for (int i = 0; i < MAX_COUNTERS; ++i)
	{
		*(void**)(pIter) = pPrev;
		pPrev = pIter;
		pIter -= 1;
	}

	g_pNextCounter = pPrev;
}

void allocCounter(Counter** ppCounter)
{
	//lock
	while (InterlockedCompareExchange(&g_counterLock, 1, 0) != 0);
	
	assert("ppCounter allocation size to small!" && g_pNextCounter != NULL);

	*ppCounter = g_pNextCounter;
	g_pNextCounter = *(void**)g_pNextCounter;
	memset(*ppCounter, 0, sizeof(Counter));

	InterlockedExchange(&g_counterLock, 0);
}


void freeCounter(Counter* counter)
{
	//lock
	while (InterlockedCompareExchange(&g_counterLock, 0, 1) != 0);
	*(void**)(counter) = g_pNextCounter;
	g_pNextCounter = counter;

	InterlockedExchange(&g_counterLock, 0);
}


void switchFiber(Fiber* pCurrFiber, Fiber* pNewFiber)
{
	pCurrFiber->status = FREE;
	pNewFiber->status = RUNNING;
	pNewFiber->currentThread = pCurrFiber->currentThread;
	
	pCurrFiber->currentThread = -1;
	pCurrFiber->currentJob = NULL;


	//push current fiber in the fiber pool
	g_pFiberPool[InterlockedIncrement(&g_fiberPoolHead)] = pCurrFiber;
	// switch to new fiber
	SwitchToFiber(pNewFiber->fiber);
}

void runNewFiber(int threadId)
{
	Fiber* fiber = g_pFiberPool[InterlockedDecrement(&g_fiberPoolHead) + 1];
	fiber->status = RUNNING;
	fiber->currentThread = threadId;
	SwitchToFiber(fiber->fiber);
}

unsigned int __stdcall threadStart(void* id)
{
	
	ConvertThreadToFiber(0);
	runNewFiber(*((int*)id));
	ConvertFiberToThread();
	return 0;
}

void __stdcall fiberRoutine(void* args)
{

	Fiber* pSelf = args;

	while (1)
	{
		
		//grab new job
		Job* pJob = NULL;
		pSelf->status = NO_JOB;
		for (;;)
		{
			// check first for local jobs ...
			pJob = popJob(&g_localJobQueues[pSelf->currentThread]);
			if (pJob != NULL)
				break;
			// ... then for global jobs
			pJob = popJob(&g_globalJobQueue);

			if (pJob != NULL)
				break;
		
			// no job available
			Sleep(10);
		}
		
		Job currentJob = *pJob;
		pSelf->currentJob = &currentJob;

		pSelf->status = RUNNING;
		currentJob.fpFunction(currentJob.pParams);

#ifdef _DEBUG
		pJob->fpFunction = (void(*)(void*))0xFEFEFEFE;
#endif
		

		// decrement counter and check wait list
		if (currentJob.pCounter != 0 && InterlockedDecrement(&currentJob.pCounter->counter) == 0)
		{
			// should be threadsafe because only one fiber will reach the counter to zero
			Counter* pCounter = currentJob.pCounter;
			if (pCounter->pWaitList != NULL)
			{
				if (pCounter->pWaitList->pNextInWaitList != NULL)
				{
					//put remaining wait list in global wait list
					//lock
					while (InterlockedCompareExchange(&g_globalWaitListLock, 0, 1) != 0);

					if (g_pGlobalWaitList == 0)
					{
						g_pGlobalWaitList = pCounter->pWaitList->pNextInWaitList;
					}
					else
					{
						Fiber* pFiber = g_pGlobalWaitList;
						// goto end of list
						while (pFiber->pNextInWaitList != NULL)
						{
							pFiber = pFiber->pNextInWaitList;
						}
						pFiber->pNextInWaitList = pCounter->pWaitList->pNextInWaitList;

					}
					InterlockedExchange(&g_globalWaitListLock, 0);

					pCounter->pWaitList = NULL;
				}

				// switch to wait fiber
				Fiber* pWaitingFiber = pCounter->pWaitList;
				// free counter
				freeCounter(pCounter);
				switchFiber(pSelf, pWaitingFiber);
				continue;
			}
		}
		else
		{
			//lock
			Fiber* pNewFiber = NULL;
			while (InterlockedCompareExchange(&g_globalWaitListLock, 0, 1) != 0);
			if (g_pGlobalWaitList != NULL)
			{
				Fiber* pNewFiber = g_pGlobalWaitList;
				g_pGlobalWaitList = g_pGlobalWaitList->pNextInWaitList;
			}
			InterlockedExchange(&g_globalWaitListLock, 0);

			if (pNewFiber != NULL)
				switchFiber(pSelf, pNewFiber);
			continue;
		}

	}
}




void initJobSystem()
{
	//find number of cores
	SYSTEM_INFO sysInfo = { 0 };
	GetSystemInfo(&sysInfo);
	g_numProcessors = sysInfo.dwNumberOfProcessors;

	//spawn fibers
	for (int i = 0; i < NUM_FIBERS; ++i)
	{
		g_fibers[i].fiber = CreateFiber(FIBER_STACK_SIZE, fiberRoutine, g_fibers + i);
		g_fibers[i].pNextInWaitList = NULL;
		g_fibers[i].status = UNUSED;
		g_fibers[i].currentThread = -1;
		g_pFiberPool[NUM_FIBERS - 1 - i] = g_fibers + i;
	}



	//init job queues
	g_globalJobQueue.front = 0;
	g_globalJobQueue.back = 0;
	g_globalJobQueue.backRdy = 0;
	g_globalJobQueue.pJobs = g_globalJobs;
	g_globalJobQueue.maxJobs = MAX_GLOBAL_JOB_QUEUE_SIZE;

	for (int i = 0; i < g_numProcessors; ++i)
	{
		g_localJobQueues[i].front = 0;
		g_localJobQueues[i].back = 0;
		g_localJobQueues[i].backRdy = 0;
		g_localJobQueues[i].pJobs = g_localJobs[i];
		g_localJobQueues[i].maxJobs = MAX_LOCAL_JOB_QUEUE_SIZE;
	}


	//spawn threadsg_threads[0] = GetCurrentThread();
	SetThreadAffinityMask(g_threads[0], 1 << 0);

	
	for (int i = 1; i < g_numProcessors; ++i)
	{	
		g_threadIds[i] = i;
		//create thread
		g_threads[i] = CreateThread(NULL, 0, threadStart, g_threadIds+i, 0, NULL);
		//lock thread to specific cpu
		SetThreadAffinityMask(g_threads[i], 1 << i);
	}

	initCounterAllocator();

}

void startMainThread()
{
	g_threadIds[MAIN_THREAD] = MAIN_THREAD;
	threadStart(g_threadIds+MAIN_THREAD);
}



void runJobsInThread(JobDecl* pJobDcls, int numJobs, Counter** ppCounter, int threadId)
{
	assert(threadId < g_numProcessors);
	pushJobs(pJobDcls, numJobs, ppCounter, &g_localJobQueues[threadId]);
}

void runJobs(JobDecl* pJobDecls, int numJobs, Counter** ppCounter)
{
	pushJobs(pJobDecls, numJobs, ppCounter, &g_globalJobQueue);
}

void waitForCounter(Counter* pCounter)
{
	Fiber* pSelf = GetFiberData();

	// push counter in wait queue
	//lock
	while (InterlockedCompareExchange(&pCounter->lock, 1, 0) != 0);
	if (pCounter->pWaitList == NULL)
	{
		pCounter->pWaitList = pSelf;
	}
	else
	{
		Fiber* pFiber = pCounter->pWaitList;
		while (pFiber->pNextInWaitList != NULL)
			pFiber = pFiber->pNextInWaitList;
		pFiber->pNextInWaitList = pSelf;
	}
	InterlockedExchange(&pCounter->lock, 0);

	// swap in new fiber
	pSelf->status = WAITING;
	int currentThread = pSelf->currentThread;
	pSelf->currentThread = -1;

	runNewFiber(currentThread);
}


void printJobSystemDebug(FILE* pFile)
{

	fprintf(pFile, "\n\n\n\n Current Running Fibers:\n");
	for (int i = 0; i < NUM_FIBERS; ++i)
	{
		if (g_fibers[i].status == RUNNING)
		{
			fprintf(pFile, "\t%2i Thread: %i Job: %s\n", i, g_fibers[i].currentThread, g_fibers[i].currentJob->pName);
		}
	}
}




#endif