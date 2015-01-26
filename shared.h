#ifndef shared_h
#define shared_h

#ifndef  _XOPEN_SOURCE
#define  _XOPEN_SOURCE
#endif

#include <stdarg.h>
#include <pthread.h>

extern pthread_key_t threadNumKey;
void initializeThreading();
void runThread();

char * timeNow();
void mkdirs(char * formattedFileName);

void stdLog(const char * str, ...);
void exitError(int num, int errorNumber, const char * str, ...);
void logWarning(int errorNumber, const char * str, ...);

void * mallocSafe(size_t bytes);
void * reallocSafe(void * existing, size_t bytes);

typedef struct {
    int processID;
    const char * command;
    int stdin;
    int stdout;
} ChildProcess;

void execPipeProcess(ChildProcess * process, const char* szCommand, int in, int out);
void closeAndWait(ChildProcess * process);

#endif // prime_shared_h
