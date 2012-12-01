#ifndef PRIME_SHARED_H
#define PRIME_SHARED_H

char * timeNow();
void exitError(int returnCode, int lerrno);
void * mallocSafe(size_t bytes);
void * reallocSafe(void * existing, size_t bytes);

#endif // PRIME_SHARED_H