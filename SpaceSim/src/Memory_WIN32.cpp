#ifdef _WIN32
#include "Memory.h"
#include <Windows.h>

void* allocPages(int numPages)
{
	return VirtualAlloc(NULL, PAGE_SIZE * numPages, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
}

void freePages(void* pages)
{
	VirtualFree(pages, 0, MEM_RELEASE);
}


#endif