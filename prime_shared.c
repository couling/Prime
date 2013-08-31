#include <time.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "prime_shared.h"

char threadString[11] = "xx";

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
	fprintf(stderr, "%s [%s] Error: %s\n", timeNow(), threadString, strerror(lerrno));
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

