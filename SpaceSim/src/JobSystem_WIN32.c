#ifdef _WIN32
#include "JobSystem.h"
#include <Windows.h>
#include <assert.h>

#define MAX_NUMBER_PROCESSORS 16
#define FIBER_STACK_SIZE (1 << 16)
#define MAX_JOBS 1024
#define MAX_COUNTERS 1024
#define IDLE_SLEEP_TIME 10


// no seperate queues for local jobs,
// store thread in fiber as bitpattern
// and check thread before popping...

// counter needs to be rewritten(counters can be deallocated too soon)...

// rewrite job queue as job list

typedef long atomic;

typedef struct Job
{
	void(*fpFunction)(void* pParams);
	void* pParams;
	char* pName;
	int threadId;
Counter* pCounter;
struct Job* pNextJob;
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
	Job* pCurrentJob;
} Fiber;

typedef struct
{
	HANDLE handle;
	int id;
	Job* pJobQueueTail;
	Fiber* pCurrentFiber;
} Thread;

typedef struct Counter
{
	Fiber* pWaitListHead;
	Fiber pWaitListZ;
	atomic counter;
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
static Thread g_threads[MAX_NUMBER_PROCESSORS];
static DWORD g_threadData;


static Job g_jobs[MAX_JOBS];
static Job* g_pFreeJob;
static Job g_dummyJob;
static Job* g_pJobQueueHead;


static Counter* g_pNextCounter;
static Counter g_counters[MAX_COUNTERS];

static atomic g_globalWaitListLock;
static Fiber* g_pWaitListHead;
static Fiber g_pWaitListTail;


static void allocCounter(Counter** ppCounter);


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
	assert("ppCounter allocation size to small!" && g_pNextCounter != NULL);

	for (;;)
	{
		Counter* pOldNextCounter = g_pNextCounter;
		*ppCounter = pOldNextCounter;
		if (InterlockedCompareExchangePointer(&g_pNextCounter, *(void**)g_pNextCounter, pOldNextCounter) == pOldNextCounter)
		{
			memset(*ppCounter, 0, sizeof(Counter));
			(*ppCounter)->pWaitListHead = &(*ppCounter)->pWaitListZ;
			return;
		}

	}
}


void freeCounter(Counter* counter)
{
	//lock
	for (;;)
	{
		Counter* pOldNextCounter = g_pNextCounter;
		*(void**)(counter) = g_pNextCounter;
		if (InterlockedCompareExchangePointer(&g_pNextCounter, counter, pOldNextCounter) == pOldNextCounter)
		{
			break;
		}
	}
}


void switchFiber(Fiber* pCurrFiber, Fiber* pNewFiber)
{
	pCurrFiber->status = FREE;
	pNewFiber->status = RUNNING;

	pCurrFiber->pCurrentJob = NULL;


	//push current fiber in the fiber pool
	g_pFiberPool[InterlockedIncrement(&g_fiberPoolHead)] = pCurrFiber;
	// switch to new fiber

	Thread* thread = TlsGetValue(g_threadData);
	thread->pCurrentFiber = pNewFiber;
	
	SwitchToFiber(pNewFiber->fiber);
}

void runNewFiber()
{
	Fiber* fiber = g_pFiberPool[InterlockedDecrement(&g_fiberPoolHead) + 1];
	fiber->status = RUNNING;

	Thread* thread = TlsGetValue(g_threadData);
	thread->pCurrentFiber = fiber;
	SwitchToFiber(fiber->fiber);
}

unsigned int __stdcall threadStart(void* threadData)
{
	TlsSetValue(g_threadData, threadData);

	ConvertThreadToFiber(0);
	runNewFiber();
	ConvertFiberToThread();
	return 0;
}

static void pushCounterWaitList(Counter* pCounter)
{
	if (pCounter->pWaitListHead == &pCounter->pWaitListZ)
		return;

	
	for (;;)
	{
		Fiber* oldWaitListHead = g_pWaitListHead;
		//pLastInWaitList->pNextInWaitList = oldWaitList;
		if (InterlockedCompareExchangePointer(&g_pWaitListHead, pCounter->pWaitListHead, oldWaitListHead) == oldWaitListHead)
		{
			oldWaitListHead->pNextInWaitList = pCounter->pWaitListZ.pNextInWaitList;
			break;
		}

	}
}

void __stdcall fiberRoutine(void* args)
{

	Fiber* pSelf = args;


	while (1)
	{
		Thread* pThread = TlsGetValue(g_threadData);
		//check waitlist
		for (;;)
		{
			Fiber* pFiber = g_pWaitListTail.pNextInWaitList;
			Fiber* pLastFiber = &g_pWaitListTail;

			while (pFiber != NULL && !(pFiber->pCurrentJob->threadId & pThread->id))
			{
				pLastFiber = pFiber;
				pFiber = pFiber->pNextInWaitList;
			}

			// no job in wait list
			if (pFiber == NULL)
				break;

			// unlink
			if (InterlockedCompareExchangePointer(&pLastFiber->pNextInWaitList, pFiber->pNextInWaitList, pFiber) == pFiber)
			{
				// wake fiber
				switchFiber(pSelf, pFiber->fiber);
			}

		}


		//grab new job
		Job* pJob = NULL;
		pSelf->status = NO_JOB;

		for (;;)
		{
			// iterate list to find next thread compatible job
			// unlink this job and set the job before that as new tail
			// if end of list is reached set the tail to the last link

			pJob = pThread->pJobQueueTail->pNextJob;
			Job* pLastJob = pThread->pJobQueueTail;

			while (pJob != NULL && !(pJob->threadId & pThread->id))
			{
				pLastJob = pJob;
				pJob = pJob->pNextJob;
			}

			// no job found
			if (pJob == NULL)
			{
				Sleep(IDLE_SLEEP_TIME);
				pThread->pJobQueueTail = pLastJob;
				continue;
			}

			assert(pJob->fpFunction != (void(*)(void*))0xFEFEFEFE);

			// unlink job
			if (InterlockedCompareExchangePointer(&pLastJob->pNextJob, pJob->pNextJob, pJob) == pJob)
			{
				pThread->pJobQueueTail = pLastJob;
				break;
			}

		}

		// execute Job
		pSelf->pCurrentJob = pJob;
		pSelf->status = RUNNING;

		pJob->fpFunction(pJob->pParams);
		
		pSelf->pCurrentJob = NULL;
		

#ifdef _DEBUG
		pJob->fpFunction = (void(*)(void*))0xFEFEFEFE;
#endif
		Counter* pCounter = pJob->pCounter;

		// dealloc job
		for (;;)
		{
			Job* pOldFreeJob = g_pFreeJob;
			pJob->pNextJob = g_pFreeJob;


			if (InterlockedCompareExchangePointer(&g_pFreeJob, pJob, pOldFreeJob) == pOldFreeJob)
				break;
		}

		// decrement counter and check wait list
		if (pCounter != 0 && InterlockedDecrement(&pCounter->counter) == 0)
		{
			// should be threadsafe because only one fiber will reach the counter to zero
			if (pCounter->pWaitListHead != &pCounter->pWaitListZ)
			{
				pushCounterWaitList(pCounter);
			} 
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
		g_pFiberPool[NUM_FIBERS - 1 - i] = g_fibers + i;
	}



	//init jobs

	for (int i = 1; i < MAX_JOBS; ++i)
	{
		g_jobs[i - 1].pNextJob = g_jobs + i;
	}
	g_pFreeJob = g_jobs;
	g_dummyJob.threadId = 0x0;
	g_pJobQueueHead = &g_dummyJob;

	//spawn threads
	g_threadData = TlsAlloc();

	for (int i = 0; i < g_numProcessors; ++i)
	{
		//create thread
		if (i == 0)
		{
			g_threads[0].handle = GetCurrentThread();
		}
		else
		{
			g_threads[i].handle = CreateThread(NULL, 0, threadStart, g_threads + i, 0, NULL);
		}

		g_threads[i].id = 0x1 << i;
		g_threads[i].pJobQueueTail = &g_dummyJob;

		//lock thread to specific cpu
		SetThreadAffinityMask(g_threads[i].handle, 1 << i);
	}
	TlsSetValue(g_threadData, g_threads + 0);

	initCounterAllocator();

}

void startMainThread()
{
	threadStart(g_threads + 0);
}



static void pushJobs(JobDecl* pJobDcls, int numJobs, Counter** ppCounter, int threadMask)
{

	//alloc counter
	if (ppCounter != NULL)
	{
		allocCounter(ppCounter);
		(*ppCounter)->counter = numJobs;
	}

	for (int i = 0; i < numJobs; ++i)
	{
		Job* pJob;
		for (;;)
		{
			pJob = g_pFreeJob;

			if (InterlockedCompareExchangePointer(&g_pFreeJob, pJob->pNextJob, pJob) == pJob)
			{
				pJob->pNextJob = NULL;
				break;
			}
		}

		pJob->fpFunction = pJobDcls[i].fpFunction;
		pJob->pParams = pJobDcls[i].pParams;
		pJob->pName = pJobDcls[i].pName;
		pJob->threadId = threadMask;
		pJob->pCounter = ppCounter == 0 ? 0 : *ppCounter;
		

		Job* pOldHead;

		for (;;)
		{
			pOldHead = g_pJobQueueHead;
			if (InterlockedCompareExchangePointer(&g_pJobQueueHead, pJob, pOldHead) == pOldHead)
			{
				pOldHead->pNextJob = g_pJobQueueHead;
				break;
			}
		}
	}
}

void runJobs(JobDecl* pJobDecls, int numJobs, Counter** ppCounter)
{
	pushJobs(pJobDecls, numJobs, ppCounter, ~0x0);
}

void runJobsInThread(JobDecl* pJobDcls, int numJobs, Counter** ppCounter, int threadId)
{
	assert(threadId < g_numProcessors);
	pushJobs(pJobDcls, numJobs, ppCounter, 0x1 << threadId);
}

void deleteCounter(Counter* pCounter)
{
	// push remaining wait list into global
	pushCounterWaitList(pCounter);

	freeCounter(pCounter);
}

void waitForCounter(Counter* pCounter)
{
	// counter already zero
	if (pCounter->counter == 0)
		return;

	Fiber* pSelf = GetFiberData();

	// push fiber in wait queue
	//lock
	pSelf->pNextInWaitList = NULL;
	for (;;)
	{
		Fiber* pOldWaitListHead = pCounter->pWaitListHead;
		if (InterlockedCompareExchangePointer(&pCounter->pWaitListHead, pSelf, pOldWaitListHead) == pOldWaitListHead)
		{
			pOldWaitListHead->pNextInWaitList= pSelf;
			break;
		}
	}

	// swap in new fiber
	pSelf->status = WAITING;
	runNewFiber();
}


void printJobSystemDebug(FILE* pFile)
{
	for (int i = 0; i < g_numProcessors; ++i)
	{
		if (g_threads[i].pCurrentFiber)
		{
			if (g_threads[i].pCurrentFiber->pCurrentJob)
				fprintf(pFile, "Thread: %i Job: %s\n", i, g_threads[i].pCurrentFiber->pCurrentJob->pName);
			else
				fprintf(pFile, "Thread: %i Idle\n", i);
		}
	}
}




#endif