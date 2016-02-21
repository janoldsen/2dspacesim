#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include "JobSystem.h"

typedef struct File File;

void fsInit(const char* pDir, int updtFreq);

File* fsCreate(const char* pName, JobDecl* pUpdateJob);
void fsDestroy(File* file);
void fsOpenRead(File* pFile);
void fsOpenWrite(File* pFile);
void fsClose(File* pFile);

void fsWrite(File* pFile, char* pBuffer, size_t size);
void fsRead(File* pFile, char* pBuffer, size_t size);
int fsReady(File* pFile);

#endif



