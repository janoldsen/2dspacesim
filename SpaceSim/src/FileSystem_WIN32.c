#include "FileSystem.h"
#include "ThreadSafety.h"
#include "Memory.h"
#include "defines.h"
#include <Windows.h>
#include <string.h>

typedef struct File
{
	HANDLE handle;
	OVERLAPPED overlapped;
	JobDecl updateJob;
	uint32 nameHash;
	const char* pName;
	File* pNext;
} File;

static uint8* g_pMem;
static size_t g_memSize;
static size_t g_memHead;
static File* g_pFreeFile;

static ReadWriteLock g_fileLock;

static HANDLE g_dir;
static const int g_updtFreq;

#define HASH_MAP_SIZE 128
static File* fileMap[HASH_MAP_SIZE];

//http://www.cse.yorku.ca/~oz/hash.html
uint32 hashStr(const char* str)
{
	uint32 hash = 5381;
	int c;
	while (c = *str++)
	{
		hash = ((hash << 5) + hash) + c;
	}

	return hash;
}

File* getFile(uint32 hash)
{
	return fileMap[hash%HASH_MAP_SIZE];
}

void setFile(uint32 hash, File* pFile)
{
	File** ppFile = &fileMap[hash%HASH_MAP_SIZE];
	while (*ppFile)
	{
		ppFile = &((*ppFile)->pNext);
	}
	*ppFile = pFile;
}

JOB_ENTRY(updateDirectory)
{
	char buffer[64] = { 0 };
	for (;;)
	{
		OVERLAPPED overlapped = { 0 };

		ReadDirectoryChangesW(g_dir, buffer, 64, TRUE, FILE_NOTIFY_CHANGE_LAST_WRITE, NULL, &overlapped, NULL);

		int numberOfBytes;
		while (!GetOverlappedResult(g_dir, &overlapped, &numberOfBytes, FALSE))
		{
			jsWait(0);
		}
#
		char* p = buffer;
		for (;;)
		{
			FILE_NOTIFY_INFORMATION* pInfo = (FILE_NOTIFY_INFORMATION*)p;

			char fileName[MAX_PATH];
			memcpy(pInfo->FileName, fileName, pInfo->FileNameLength);
			fileName[pInfo->FileNameLength] = 0;

			uint32 hash = hashStr(fileName);
			
			File* file = getFile(hash);
			while (file != 0)
			{
				if (strcmp(file->pName, fileName) == 0)
				{
					if (file->updateJob.fpFunction != NULL)
						jsRunJobs(&file->updateJob, 1, 0);
					break;
				}
				file = file->pNext;
			}


			if (pInfo->NextEntryOffset == 0)
				break;
			p += pInfo->NextEntryOffset;
		}

		jsWait(g_updtFreq);
	}

}

void fsInit(const char* pDir, int updtFreq)
{
	g_pMem = allocPages(1);
	g_memSize = PAGE_SIZE;
	memset(g_pMem, 0, g_memSize);

	g_dir = CreateFileA(pDir, GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
		0, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, NULL);

	


	JobDecl updateDirJob;
	updateDirJob.fpFunction = updateDirectory;
	updateDirJob.pName = "updateFileDirectoy";
	updateDirJob.pParams = 0;
}

File* fsCreate(const char* pName, JobDecl* pUpdateJob)
{
	uint32 hash = hashStr(pName);
	File* pNewFile;

	if (g_pFreeFile)
	{
		pNewFile = g_pFreeFile;
		g_pFreeFile = g_pFreeFile->pNext;
	}
	else
	{
		if (g_memHead + sizeof(File) > g_memSize)
		{
			g_pMem = allocPages(1);
			g_memHead = 0;
		}

		pNewFile = (File*)(g_pMem+g_memHead);
		g_memHead += sizeof(File);
	}

	pNewFile->pName = pName;
	pNewFile->nameHash = hash;
	if (pUpdateJob)
		pNewFile->updateJob = *pUpdateJob;

	setFile(hash, pNewFile);

	return pNewFile;
}

void fsDestroy(File* pFile)
{
	File* pLastFile = getFile(pFile->nameHash);

	while (pLastFile)
	{
		if (pLastFile->pNext = pFile)
		{
			pLastFile->pNext = pFile->pNext;
			break;
		}
	}

	pFile->pNext = g_pFreeFile;
	g_pFreeFile = pFile;
}

void fsOpenRead(File* pFile)
{
	pFile->handle = CreateFile(pFile->pName, GENERIC_READ | GENERIC_WRITE, 
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		0, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, 0);
}

void fsOpenWrite(File* pFile)
{
	pFile->handle = CreateFile(pFile->pName, GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, 0);
}

void fsClose(File* pFile)
{
	CloseHandle(pFile->handle);
}

void fsWrite(File* pFile, char* pBuffer, size_t size)
{
	WriteFile(pFile->handle, pBuffer, size, NULL, &pFile->overlapped);
}

void fsRead(File* pFile, char* pBuffer, size_t size)
{
	ReadFile(pFile->handle, pBuffer, size, NULL, &pFile->overlapped);
}

int fsReady(File* pFile)
{
	return HasOverlappedIoCompleted(&pFile->overlapped);
}