#ifndef PRIME_SHARED_H
#define PRIME_SHARED_H

#include <stdlib.h>

char * timeNow();
void exitError(int returnCode, int lerrno);
void * mallocSafe(size_t bytes);
void * reallocSafe(void * existing, size_t bytes);

void writePrimeRaw(FILE * file, unsigned char * buffer,  size_t mapSize, Prime offset);
void writePrimeText(FILE * file, unsigned char * buffer,  size_t mapSize, Prime offset);
void writePrimePrime(FILE * file, unsigned char * buffer,  size_t mapSize, Prime offset);

#endif // PRIME_SHARED_H
