#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <errno.h>

#include "primeshared.h"
 
char * timeNow() {
    time_t rawtime;
    struct tm * timeinfo;
    time ( &rawtime );
    timeinfo = localtime ( &rawtime );
    static char t[100];
    strftime (t,100,"%a %b %d %H:%M:%S %Y",timeinfo);
    return t;
}



void exitError(int returnCode, int lerrno) {
    fprintf(stderr, "%s Error: %s\n", timeNow(), strerror(lerrno));
    exit(returnCode);
} 



void * mallocSafe(size_t bytes) {
    void * result = malloc(bytes);
    if (!result) exitError(255, errno);
    return result;
}



void * reallocSafe(void * existing, size_t bytes) {
    void * result = realloc(existing, bytes);
    if (!result) exitError(255, errno);
    return result;
}



void writePrimeRaw(FILE * file, unsigned char * buffer,  size_t mapSize, Prime offset) {
}



void writePrimeText(FILE * file, unsigned char * buffer,  size_t mapSize, Prime offset) {
}



void writePrimePrime(FILE * file, unsigned char * buffer,  size_t mapSize, Prime offset) {
}
