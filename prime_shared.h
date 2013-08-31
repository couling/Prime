#ifndef prime_shared_h
#define prime_shared_h

char * timeNow();
void exitError(int returnCode, int lerrno);
void * mallocSafe(size_t bytes);
void * reallocSafe(void * existing, size_t bytes);

extern char threadString[11];

#endif // prime_shared_h
