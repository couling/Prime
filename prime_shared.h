#ifndef prime_shared_h
#define prime_shared_h

#include <stdarg.h>

// Config parameters
extern char * startValueString;
extern char * startValueScale;
extern char * endValueString;
extern char * endValueScale;
extern char * chunkSizeString;
extern char * chunkSizeScale;

extern int threadCount;
extern int threadNum;
extern int singleThread;
extern char threadString[11];

extern char * fileDir;
extern char * filePrefix;
extern char * fileSuffix;
extern char * fileInfix;
extern int useStdout;

extern int silent;
extern int verbose;


// For for file based initialisation
extern int initializeOnly; // create an initialisation file
extern char * * files;
extern int fileCount;


char * timeNow();
void exitError(int returnCode, int lerrno);

void printToStdErr(char * str, ...);
void raiseError(int num, char * str, ...);

void * mallocSafe(size_t bytes);
void * reallocSafe(void * existing, size_t bytes);

void parseArgs(int argC, char ** argV);

#endif // prime_shared_h
