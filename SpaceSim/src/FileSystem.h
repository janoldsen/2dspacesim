#ifndef FILESYSTEM_H
#define FILESYSTEM_H

typedef struct File File;

File* fsOpen();
void fsClose(File);


#endif



