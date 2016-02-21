#ifndef MEMORY_H
#define MEMORY_H

#define PAGE_SIZE 4096

void* allocPages(int numPages);
void freePages(void* pages);

#endif
