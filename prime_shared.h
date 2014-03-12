#ifndef prime_shared_h
#define prime_shared_h

#include <stdlib.h> 
#include <stdarg.h>
#include <pthread.h>
#include <limits.h>

// Set up the architecture

#ifndef PRIME_SIZE
	#define PRIME_SIZE 64
#endif

#if PRIME_SIZE == 32
	#define PRIME_STRING_SIZE 11
#elif PRIME_SIZE == 64
	#define PRIME_STRING_SIZE 21
#elif PRIME_SIZE == 96
	#define PRIME_STRING_SIZE 30
#elif PRIME_SIZE == 128
	#define PRIME_STRING_SIZE 40
#else
    #error PRIME_SIZE is invalid
#endif


#ifndef PRIME_ARCH
	#define PRIME_ARCH INT
#endif


#if PRIME_ARCH = INT
	#include "prime_64.h"
#elif PRIME_ARCH = GMP
	#include "primegmp.h"
#endif


typedef char PrimeString[PRIME_STRING_SIZE];

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
extern char * fileName;
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

void stdLog(char * str, ...);
void exitError(int num, int errorNumber, char * str, ...);

void * mallocSafe(size_t bytes);
void * reallocSafe(void * existing, size_t bytes);

void parseArgs(int argC, char ** argV);

#endif // prime_shared_h
