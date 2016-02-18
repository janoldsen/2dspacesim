#ifdef _WIN32
#include "JobSystem.h"
#include <Windows.h>
#include <assert.h>
#include <time.h>

#define MAX_NUMBER_PROCESSORS 16
#define FIBER_STACK_SIZE (1 << 16)
#define MAX_JOBS 64
#define MAX_COUNTERS 1024
#define IDLE_SLEEP_TIME 10
#define MAX_WAITING_FIBERS 64

//#define DEBUG_OUTPUT


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
	Job** ppJobQueueTail;
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

static atomic g_globalWaitCounterListLock;
static Fiber* g_pWaitCounterListHead;
static Fiber g_pWaitCounterListTail;

typedef struct WaitingFiber
{
	uint64 time;
	Fiber* pFiber;
	struct WaitingFiber* pNextWaitingFiber;
} WaitingFiber;

static uint64 g_PerformanceFrequency;
static WaitingFiber g_WaitingFibers[MAX_WAITING_FIBERS];
static atomic g_WaitingFiberHead;
static WaitingFiber* g_pWaitListHead;
static WaitingFiber g_WaitListTail;

#ifdef DEBUG_OUTPUT
static FILE* g_pDebugLog;
#endif


static void allocCounter(Counter** ppCounter);


static void initCounterAllocator()
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

static void allocCounter(Counter** ppCounter)
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


static void switchFiber(Fiber* pCurrFiber, Fiber* pNewFiber)
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

static void runNewFiber()
{
	Fiber* fiber = g_pFiberPool[InterlockedDecrement(&g_fiberPoolHead) + 1];
	fiber->status = RUNNING;

	Thread* thread = TlsGetValue(g_threadData);
	thread->pCurrentFiber = fiber;
	SwitchToFiber(fiber->fiber);
}

unsigned int __stdcall threadStart(void* threadData)
{
#ifdef _DEBUG
	//set thread debug name
	struct {
		DWORD type;
		LPCSTR pName;
		DWORD threadID;
		DWORD flags;
	} info;

	char name[64];
	sprintf(name, "thread_%i", ((Thread*)threadData) - g_threads);

	info.type = 0x1000;
	info.pName = name;
	info.threadID = -1;
	info.flags = 0;

	RaiseException(0x406D1388, 0, sizeof(info), (const ULONG_PTR*)&info);
#endif

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
		Fiber* oldWaitListHead = g_pWaitCounterListHead;
		//pLastInWaitList->pNextInWaitList = oldWaitList;
		if (InterlockedCompareExchangePointer(&g_pWaitCounterListHead, pCounter->pWaitListHead, oldWaitListHead) == oldWaitListHead)
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

	FIBER_CHECK:
		//check timer waitlist
		for (;;)
		{
			WaitingFiber* pWaitingFiber = g_WaitListTail.pNextWaitingFiber;

			uint64 perfCounter;
			QueryPerformanceCounter((LARGE_INTEGER*)&perfCounter);
			if (pWaitingFiber != NULL && perfCounter > pWaitingFiber->time)
			{
				if (InterlockedCompareExchangePointer(&g_WaitListTail.pNextWaitingFiber, pWaitingFiber->pNextWaitingFiber, pWaitingFiber) == pWaitingFiber)
				{
					switchFiber(pSelf, pWaitingFiber->pFiber);
				}
			}
			else
			{
				break;
			}

		}


		//check counter waitlist
		for (;;)
		{
			Fiber* pFiber = g_pWaitCounterListTail.pNextInWaitList;
			Fiber* pLastFiber = &g_pWaitCounterListTail;

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
			Job** ppJobLink = pThread->ppJobQueueTail;

			pJob = *pThread->ppJobQueueTail;

			while (pJob->pNextJob != NULL && !(pJob->threadId & pThread->id))
			{
				ppJobLink = &(pJob->pNextJob);
				pJob = *ppJobLink;
			}


			// no job found
			if (pJob->pNextJob == NULL)
			{
				pThread->ppJobQueueTail = ppJobLink;
				Sleep(IDLE_SLEEP_TIME);
				goto FIBER_CHECK;
			}

			assert(pJob->fpFunction != (void(*)(void*))0xFEFEFEFE);



			// unlink job
			if (InterlockedCompareExchangePointer(ppJobLink, pJob->pNextJob, pJob) == pJob)
			{
				for (int i = 0; i < g_numProcessors; ++i)
				{
					InterlockedCompareExchangePointer((void**)&g_threads[i].ppJobQueueTail, (void*)ppJobLink, (void*)&pJob->pNextJob);
				}
#ifdef _DEBUG
				pJob->pNextJob = (Job*)0xBAD;
#endif
				pThread->ppJobQueueTail = ppJobLink;
				break;
			}

		}

#ifdef DEBUG_OUTPUT
		fprintf(g_pDebugLog, "%lld \t new job(%s) started in thread %i. new job tail: %i\n", time(NULL), pJob->pName, pThread - g_threads, pThread->pJobQueueTail - g_jobs);
#endif

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




void jsInit()
{
#ifdef DEBUG_OUTPUT
	g_pDebugLog = fopen("JS_DEBUG.log", "w");
#endif 

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


	//init waiting fibers
	memset(g_WaitingFibers, 0, sizeof(g_WaitingFibers));
	g_WaitingFiberHead = 0;
	g_WaitListTail.pFiber = NULL;
	g_WaitListTail.pNextWaitingFiber = NULL;
	g_WaitListTail.time = 0;

	//get performance frequency
	QueryPerformanceFrequency((LARGE_INTEGER*)&g_PerformanceFrequency);

	//init jobs
	for (int i = 2; i < MAX_JOBS; ++i)
	{
		g_jobs[i - 1].pNextJob = g_jobs + i;
	}
	g_pFreeJob = g_jobs + 1;
	g_pJobQueueHead = g_jobs;
	g_dummyJob.threadId = 0x0;
	g_dummyJob.pNextJob = g_pJobQueueHead;
	g_dummyJob.pName = "__dummy";

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
		g_threads[i].ppJobQueueTail = &(g_dummyJob.pNextJob);



		//lock thread to specific cpu
		SetThreadAffinityMask(g_threads[i].handle, 1 << i);
	}
	TlsSetValue(g_threadData, g_threads + 0);

	initCounterAllocator();

#ifdef DEBUG_OUTPUT
	fprintf(g_pDebugLog, "%lld \t jobsystem started\n", time(NULL));
#endif

}

void jsStartMainThread()
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
		Job* pNewHead;
		for (;;)
		{
			pNewHead = g_pFreeJob;

			if (InterlockedCompareExchangePointer(&g_pFreeJob, pNewHead->pNextJob, pNewHead) == pNewHead)
			{
				pNewHead->pNextJob = NULL;
				break;
			}
		}


		for (;;)
		{
			pJob = g_pJobQueueHead;
			if (InterlockedCompareExchangePointer(&g_pJobQueueHead, pNewHead, pJob) == pJob)
			{
				break;
			}
		}

		pJob->fpFunction = pJobDcls[i].fpFunction;
		pJob->pParams = pJobDcls[i].pParams;
		pJob->pName = pJobDcls[i].pName;
		pJob->threadId = threadMask;
		pJob->pCounter = ppCounter == 0 ? 0 : *ppCounter;
		MemoryBarrier();
		pJob->pNextJob = pNewHead;
	}

#ifdef DEBUG_OUTPUT
	fprintf(g_pDebugLog, "%lld \t %i new jobs added (%s %x) \n\t\t new job head: %i\n", time(NULL), numJobs, pJobDcls->pName, threadMask, g_pJobQueueHead - g_jobs);
#endif


}

void jsRunJobs(JobDecl* pJobDecls, int numJobs, Counter** ppCounter)
{
	pushJobs(pJobDecls, numJobs, ppCounter, ~0x0);
}

void jsRunJobsInThread(JobDecl* pJobDcls, int numJobs, Counter** ppCounter, int threadId)
{
	//assert(threadId < g_numProcessors);
	pushJobs(pJobDcls, numJobs, ppCounter, 0x1 << threadId);
}

void jsDeleteCounter(Counter* pCounter)
{
	// push remaining wait list into global
	pushCounterWaitList(pCounter);

	freeCounter(pCounter);
}

void jsWaitForCounter(Counter* pCounter)
{
	// counter already zero
	if (pCounter->counter == 0)
		return;

	Fiber* pSelf = GetFiberData();

	// push fiber in wait queue
	pSelf->pNextInWaitList = NULL;
	for (;;)
	{
		Fiber* pOldWaitListHead = pCounter->pWaitListHead;
		if (InterlockedCompareExchangePointer(&pCounter->pWaitListHead, pSelf, pOldWaitListHead) == pOldWaitListHead)
		{
			pOldWaitListHead->pNextInWaitList = pSelf;
			break;
		}
	}

	// swap in new fiber
	pSelf->status = WAITING;
	runNewFiber();
}

uint32 jsWait(uint32 ms)
{

	Fiber* pSelf = GetFiberData();
	WaitingFiber* pWaitingFiber;
	for (;;)
	{
		int oldWaitingFiberHead = g_WaitingFiberHead;
		int waitingFiberHead = g_WaitingFiberHead;

		do
		{
			waitingFiberHead = (waitingFiberHead + 1) % MAX_WAITING_FIBERS;
		} while (g_WaitingFibers[waitingFiberHead].pFiber != NULL);

		if (InterlockedCompareExchange(&g_WaitingFiberHead, waitingFiberHead, oldWaitingFiberHead) == oldWaitingFiberHead)
		{
			pWaitingFiber = g_WaitingFibers + waitingFiberHead;
			break;
		}
	}

	pWaitingFiber->pFiber = pSelf;

	uint64 perfCounter;
	QueryPerformanceCounter((LARGE_INTEGER*)&perfCounter);
	pWaitingFiber->time = perfCounter + ms*(g_PerformanceFrequency / 1000);

	//sort into waiting list
	for (;;)
	{
		WaitingFiber* pLastFiber = &g_WaitListTail;
		WaitingFiber* pNextWaitingFiber = g_WaitListTail.pNextWaitingFiber;

		while (pNextWaitingFiber != NULL && pNextWaitingFiber->time < pWaitingFiber->time)
		{
			pLastFiber = pNextWaitingFiber;
			pNextWaitingFiber = pNextWaitingFiber->pNextWaitingFiber;
		}

		pWaitingFiber->pNextWaitingFiber = pNextWaitingFiber;

		if (InterlockedCompareExchangePointer(&pLastFiber->pNextWaitingFiber, pWaitingFiber, pNextWaitingFiber) == pNextWaitingFiber)
		{
			break;
		}
	}

	pSelf->status = WAITING;
	runNewFiber();

	QueryPerformanceCounter((LARGE_INTEGER*)&perfCounter);
	uint64 elapsedTime = (perfCounter - pWaitingFiber->time) * 1000 / g_PerformanceFrequency;
	pWaitingFiber->pFiber = NULL;
	return (uint32)elapsedTime;
}

void jsPrintDebug(FILE* pFile)
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

int jsNumThreads()
{
	return g_numProcessors;
}


#endif