#ifndef prime_shared_h
#define prime_shared_h

#include <stdlib.h> 
#include <stdarg.h>
#include <pthread.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

// Set some defaults
#ifndef PRIME_SIZE
    #define PRIME_SIZE 64
#endif

#if ! defined PRIME_ARCH_INT && ! defined PRIME_ARCH_GMP
    #define PRIME_ARCH_INT
#endif

#ifndef PRIME_PROGRAM_NAME
	#define PRIME_PROGRAM_NAME prime
#endif

#ifndef PRIME_PROGRAM_VERSION
	#define PRIME_PROGRAM_VERSION unknown
#endif

// Set up the architecture

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

#if defined PRIME_ARCH_INT
    #include "prime_64.h"
#elif defined PRIME_ARCH_GMP
    #include "prime_gmp.h"
#else
    #error PRIME_ARCH is invalid
#endif

#ifndef PRIME_PROGRAM_NAME
    #define PRIME_PROGRAM_NAME prime
#endif

#ifndef PRIME_PROGRAM_VERSION
    #define PRIME_PROGRAM_VERSION Unknown
#endif

typedef char PrimeString[PRIME_STRING_SIZE];

// Config parameters
extern Prime startValue;
extern Prime endValue;
extern Prime chunkSize;
extern Prime lowPrimeMax;

extern int threadCount;

extern char * initFileName;
extern char * fileName;
extern int singleFile;
extern int useStdout;
extern int fileType;
extern char ** inputFiles;
extern int inputFileCount;

#define FILE_TYPE_TEXT 't'
#define FILE_TYPE_SYSTEM_BINARY 'b'
#define FILE_TYPE_COMPRESSED_BINARY 'c'

#ifndef STRIP_LOGGING
extern int silent;
extern int verbose;
#else
#define silent 1
#define verbose 0
#endif

char * formatFileNamePart(char * formattedFileName, int bufferSize, const char * source, Prime from, Prime to);
char * formatFileName( char * formattedFileName, int bufferSize, Prime from, Prime to);
int openFileForPrime(Prime from, Prime to);
void closeFileForPrime(int fd);

void parseArgs(int argC, char ** argV);
char * getVersion();
#endif // prime_shared_h
