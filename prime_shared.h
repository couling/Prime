#ifndef prime_shared_h
#define prime_shared_h

#include <stdlib.h> 
#include <stdarg.h>
#include <pthread.h>

// Config parameters
extern char * startValueString;
extern char * startValueScale;
extern char * endValueString;
extern char * endValueScale;
extern char * chunkSizeString;
extern char * chunkSizeScale;

extern int threadCount;
extern int singleThread;
extern pthread_key_t threadNumKey;

extern char * fileDir;
extern char * filePrefix;
extern char * fileSuffix;
extern char * fileInfix;
extern int singleFile;
extern int useStdout;
extern int fileType;

#define FILE_TYPE_TEXT 't'
#define FILE_TYPE_SYSTEM_BINARY 'b'
#define FILE_TYPE_COMPRESSED_BINARY 'c'

extern int silent;
extern int verbose;


// For for file based initialisation
extern char * * files;
extern int fileCount;

char * timeNow();

void stdLog(char * str, ...);
void exitError(int num, int errorNumber, char * str, ...);

void * mallocSafe(size_t bytes);
void * reallocSafe(void * existing, size_t bytes);

void parseArgs(int argC, char ** argV);

#endif // prime_shared_h
