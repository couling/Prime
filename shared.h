#ifndef shared_h
#define shared_h

extern pthread_key_t threadNumKey;
void initializeThreading();

char * timeNow();
void mkdirs(char * formattedFileName);

void stdLog(char * str, ...);
void exitError(int num, int errorNumber, char * str, ...);
void logWarning(int errorNumber, char * str, ...);

void * mallocSafe(size_t bytes);
void * reallocSafe(void * existing, size_t bytes);

#endif // prime_shared_h