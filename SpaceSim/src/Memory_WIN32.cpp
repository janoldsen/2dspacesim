#ifdef _WIN32
#include "Memory.h"
#include <Windows.h>

void* allocPages(int numPages)
{
	return VirtualAlloc(NULL, 4096 * numPages, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
}

void freePages(void* pages)
{
	VirtualFree(pages, 0, MEM_RELEASE);
}


#endif