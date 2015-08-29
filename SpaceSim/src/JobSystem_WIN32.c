#ifdef _WIN32
#include "JobSystem.h"
#include <Windows.h>
#include <assert.h>

#define MAX_NUMBER_PROCESSORS 16
#define FIBER_STACK_SIZE (1 << 16)
#define MAX_JOB_QUEUE_SIZE 1024
#define MAX_COUNTERS 256

typedef long atomic;


enum
{
	RUNNING,
	WAITING,
	FREE,
	UNUSED,
} typedef FiberStatus;

typedef struct Fiber
{
	LPVOID fiber;
	struct Fiber* pNextInWaitList;
	FiberStatus status;
} Fiber;


typedef struct Counter
{
	Fiber* pWaitList;
	atomic counter;
	atomic lock;
} Counter;

typedef struct
{
	void(*fpFunction)(void* pParams);
	void* pParams;
	Counter* pCounter;
} Job;



static atomic g_fiberPoolHead = NUM_FIBERS - 1;
static Fiber* g_pFiberPool[NUM_FIBERS];
static Fiber g_fibers[NUM_FIBERS];

static int g_numProcessors;
static HANDLE g_threads[MAX_NUMBER_PROCESSORS];

static atomic g_jobQueueFront = 0;
static atomic g_jobQueueBack = 0;
static atomic g_jobLock = 0;
static Job g_jobs[MAX_JOB_QUEUE_SIZE];


static atomic g_counterLock = 0;
static Counter* g_pNextCounter;
static Counter g_counters[MAX_COUNTERS];

static atomic g_globalWaitListLock;
static Fiber* g_pGlobalWaitList;

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
	while (InterlockedCompareExchange(&g_counterLock, 0, 1) != 0);
	
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

	//push current fiber in the fiber pool
	g_pFiberPool[InterlockedIncrement(&g_fiberPoolHead)] = pCurrFiber;
	// switch to new fiber
	SwitchToFiber(pNewFiber->fiber);
}

void runNewFiber()
{
	Fiber* fiber = g_pFiberPool[InterlockedDecrement(&g_fiberPoolHead) + 1];
	fiber->status = RUNNING;
	SwitchToFiber(fiber->fiber);
}

unsigned int __stdcall threadStart(void* args)
{
	
	ConvertThreadToFiber(0);
	runNewFiber();
	ConvertFiberToThread();
	return 0;
}

void __stdcall fiberRoutine(void* args)
{

	Fiber* pSelf = args;

	while (1)
	{
		if (g_jobQueueFront == g_jobQueueBack || g_jobLock > 0)
		{
			Sleep(10);
			continue;
		}
		//grab new job
		int i = InterlockedIncrement(&g_jobQueueFront)-1;
		//wrap around
		InterlockedCompareExchange(&g_jobQueueFront, 0, MAX_JOB_QUEUE_SIZE);
		
		Job job = g_jobs[i];
		g_jobs[i].fpFunction = (void(*)(void*))0xFEFEFEFE;
		job.fpFunction(job.pParams);

		// decrement counter and check wait list
		if (job.pCounter != 0 && InterlockedDecrement(&job.pCounter->counter) == 0)
		{
			// should be threadsafe because only one fiber will reach the counter to zero
			Counter* pCounter = job.pCounter;
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
				// firee counter
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


	//spawn fibers
	for (int i = 0; i < NUM_FIBERS; ++i)
	{
		g_fibers[i].fiber = CreateFiber(FIBER_STACK_SIZE, fiberRoutine, g_fibers + i);
		g_fibers[i].pNextInWaitList = NULL;
		g_fibers[i].status = UNUSED;
		g_pFiberPool[NUM_FIBERS - 1 - i] = g_fibers + i;
	}


	//spawn threads
	
	//find number of cores
	SYSTEM_INFO sysInfo = { 0 };
	GetSystemInfo(&sysInfo);
	g_numProcessors = sysInfo.dwNumberOfProcessors;

	g_threads[0] = GetCurrentThread();
	SetThreadAffinityMask(g_threads[0], 1 << 0);

	for (int i = 1; i < g_numProcessors; ++i)
	{
		//create thread
		g_threads[i] = CreateThread(NULL, 0, threadStart, 0, 0, NULL);
		//lock thread to specific cpu
		SetThreadAffinityMask(g_threads[i], 1 << i);
	}


	// init counter alloc
	initCounterAllocator();

}

void startMainThread()
{
	threadStart(NULL);
}

void runJobs(JobDecl* jobDecls, int numJobs, Counter** ppCounter)
{
	int jobstart;
	InterlockedIncrement(&g_jobLock);
	int newBack = InterlockedAdd(&g_jobQueueBack, numJobs);
	jobstart = newBack - numJobs;
	
	while (newBack >= MAX_JOB_QUEUE_SIZE)
	{
		if (InterlockedCompareExchange(&g_jobQueueBack, (newBack%MAX_JOB_QUEUE_SIZE), newBack) == newBack)
		{
			break;
		}
		newBack = InterlockedAdd(&g_jobQueueBack, numJobs);
	}


	//aloc counter
	if (ppCounter != NULL)
	{
		allocCounter(ppCounter);
		(*ppCounter)->counter = numJobs;
	}

	for (int i = 0; i < numJobs; ++i)
	{
		int idx = (jobstart+i) % MAX_JOB_QUEUE_SIZE;
		g_jobs[idx].fpFunction = jobDecls[i].fpFunction;
		g_jobs[idx].pParams = jobDecls[i].pParams;
		g_jobs[idx].pCounter = ppCounter == 0 ? 0 : *ppCounter;
	}
	InterlockedDecrement(&g_jobLock);
}

void waitForCounter(Counter* pCounter)
{
	Fiber* pSelf = GetFiberData();

	// push counter in wait queue
	//lock
	while (InterlockedCompareExchange(&pCounter->lock, 0, 1) != 0);
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
	runNewFiber();
}







#endif