#include "shared.h"

#include <time.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <stdarg.h>
#include <pthread.h>

#include <sys/file.h>
#include <sys/stat.h>


// For thread local storage
pthread_key_t threadNumKey;



void initializeThreading() {
    pthread_key_create(&threadNumKey, NULL);
}



char * timeNow() {
    time_t rawtime;
    struct tm * timeinfo;
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    static char t[100];
    strftime(t, 100, "%a %b %d %H:%M:%S %Y", timeinfo);
    return t;
}



void stdLog(const char * str, ...) {
    int * threadNumPt = pthread_getspecific(threadNumKey);
    int threadNum = threadNumPt ? *threadNumPt : 0;
    flockfile(stderr);
    fprintf(stderr, "%s [%02d] ", timeNow(), threadNum);
    va_list ap;
    va_start(ap, str);
    vfprintf(stderr, str, ap);
    va_end(ap);
    fprintf(stderr, "\n");
    funlockfile(stderr);
}



void exitError(int num, int errorNumber, const char * str, ...) {
    int * threadNumPt = pthread_getspecific(threadNumKey);
    int threadNum = threadNumPt ? *threadNumPt : 0;
    flockfile(stderr);
    fprintf(stderr, "%s [%02d] Error: ", timeNow(), threadNum);
    va_list ap;
    va_start(ap, str);
    vfprintf(stderr, str, ap);
    va_end(ap);
    fprintf(stderr, "\n");
    if (errorNumber)
        fprintf(stderr, "%s [%02d] Error: %s\n", timeNow(), threadNum, strerror(errorNumber));
    funlockfile(stderr);
    exit(num);
}



void logWarning(int errorNumber, const char * str, ...) {
    int * threadNumPt = pthread_getspecific(threadNumKey);
    int threadNum = threadNumPt ? *threadNumPt : 0;
    flockfile(stderr);
    fprintf(stderr, "%s [%02d] Warning: ", timeNow(), threadNum);
    va_list ap;
    va_start(ap, str);
    vfprintf(stderr, str, ap);
    va_end(ap);
    fprintf(stderr, "\n");
    if (errorNumber)
        fprintf(stderr, "%s [%02d] Warning: %s\n", timeNow(), threadNum, strerror(errorNumber));
    funlockfile(stderr);
}



void * mallocSafe(size_t bytes) {
    void * result = malloc(bytes);
    if (!result) exitError(255, errno, "Could not allocate to %zd bytes", bytes);
    return result;
}



void * reallocSafe(void * existing, size_t bytes) {
    void * result = realloc(existing, bytes);
    if (!result) exitError(255, errno, "Could not reallocate to %zd bytes", bytes);
    return result;
}



void mkdirs(char * formattedFileName) {
    // Create necessary directories
    for (char * cptr = formattedFileName; *cptr != '\0'; ++cptr) {
        if (*cptr == '/') {
            *cptr = '\0';
            if (!mkdir(formattedFileName, S_IRWXU)) {
                stdLog("Created directory for output: %s", formattedFileName);
            }
            *cptr = '/';
        }
    }
}
