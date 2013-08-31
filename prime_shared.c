#include <time.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <stdarg.h>

#include "prime_shared.h"

// Config parameters
char * startValueString = "0";
char * startValueScale = "1";
char * endValueString = "1";
char * endValueScale = "1000000000";
char * chunkSizeString = "1";
char * chunkSizeScale = "1000000000";

int threadCount = 1;
int threadNum = 0;
int singleThread = 0;
char threadString[11] = "xx";

char * fileDir = ".";
char * filePrefix = "prime.";
char * fileSuffix = ".txt";
char * fileInfix = "-";
int useStdout = 1;

int silent = 0;
int verbose = 0;

// For for file based initialisation
int initializeOnly = 0; // create an initialisation file
char * * files;
int fileCount;



void printToStdErr(char * str, ...) {
	fprintf(stderr, "%s [%s] ", timeNow(), threadString);
	va_list ap;
	va_start(ap, str);
	vfprintf(stderr, str, ap);
	va_end(ap);
}



void raiseError(int num, char * str, ...) {
	fprintf(stderr, "%s [%s] Error: ", timeNow(), threadString);
    va_list ap;
    va_start(ap, str);
    vfprintf(stderr, str, ap);
    va_end(ap);
	exit(num);
}



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

void parseArgs(int argC, char ** argV) {

    static struct option longOptions[] =
            {
                {"start",  required_argument, 0, 'o'},
                {"end",  required_argument, 0, 'O'},
                {"start-thousand", required_argument, 0, 'k'},
                {"end-thousand", required_argument, 0, 'K'},
                {"start-million",  required_argument, 0, 'm'},
                {"end-million",  required_argument, 0, 'M'},
                {"start-billion",  required_argument, 0, 'g'},
                {"end-billion",  required_argument, 0, 'G'},
                {"start-trillion",  required_argument, 0, 't'},
                {"end-trillion",  required_argument, 0, 'T'},
                {"chunk-million", required_argument, 0, 'c'},
                {"chunk-billion", required_argument, 0, 'C'},
                {"prefix", required_argument, 0, 1000},
                {"suffix", required_argument, 0, 1001},
                {"infix", required_argument, 0, 1002},
                {"dir", required_argument, 0, 1003},
                {"thread-count", required_argument, 0, 'x'},
                {"thread-num", required_argument, 0, 'X'},
                {"silent", no_argument, 0, 's'},
                {"verbose", no_argument, 0, 'v'},
                {"file", no_argument, 0, 'f'},
                {"initialize-only", no_argument, 0, 'i'},
                {0, 0, 0, 0}
            };
    static char * shortOptions ="O:o:K:k:M:m:G:g:T:t:C:c:p:X:x:svfi";

    int givenOption;
    // do not allow getopt_long to print an error to stdout if an invalid option is found
    opterr = 0;
    while ((givenOption = getopt_long (argC, argV, shortOptions, longOptions, NULL)) != -1) {
        int threadingNumber = 0;
        switch (givenOption) {
            case 'o': startValueString = optarg; startValueScale = "1"; break;
            case 'O': endValueString = optarg; endValueScale = "1"; break;
            case 'k': startValueString = optarg; startValueScale = "1000"; break;
            case 'K': endValueString = optarg; endValueScale = "1000"; break;
            case 'm': startValueString = optarg; startValueScale = "1000000"; break;
            case 'M': endValueString = optarg; endValueScale = "1000000"; break;
            case 'g': startValueString = optarg; startValueScale = "1000000000"; break;
            case 'G': endValueString = optarg; endValueScale = "1000000000"; break;
            case 't': startValueString = optarg; startValueScale = "1000000000000"; break;
            case 'T': endValueString = optarg; endValueScale = "1000000000000"; break;
            case 'c': chunkSizeString = optarg; chunkSizeScale = "1000000"; break;
            case 'C': chunkSizeString = optarg; chunkSizeScale = "1000000000"; break;
            case 1000: filePrefix = optarg; break;
            case 1001: fileSuffix = optarg; break;
            case 1002: fileInfix = optarg; break;
            case 1003: fileDir = optarg; break;
            case 'x': threadingNumber = 'x'; break;
            case 'X': threadingNumber = 'X'; break;
            case 's': silent = 1; verbose = 0; break;
            case 'v': verbose = !silent; break;
            case 'f': useStdout = 0; break;
            case 'i': initializeOnly = 1; break;
            case '?':
                if (optopt) {
                    fprintf(stderr, "%s [%s] Error: option -%c\n",
                        timeNow(), threadString, optopt);
                }
                else {
                    fprintf(stderr, "%s [%s] Error: option %s\n",
                        timeNow(), threadString, argV[optind-1]);
                }
                exit(1);
                break;
        }

        if (threadingNumber) {
            long long value;
            char * endptr;
            value = strtoll(optarg, &endptr, 10);
            if (*endptr || value <= 0 || value > 1000) {
                fprintf(stderr, "%s [%s] Error: threading number %s\n",
                    timeNow(), threadString, optarg);
                exit(1);
            }
            if (threadingNumber == 'x') {
                threadCount = value;
            }
            else if (threadingNumber == 'X') {
                threadNum = value;
            }
        }
    }

    // We default to multithreading with 1 thread...
    // This is more normally known as single threading so
    // lets not waste time with the multithreading functions.
    if (threadCount == 1) singleThread = 1;

    int errorFlag = 0;

    if (strchr(filePrefix, '/') || strchr(fileInfix, '/') || strchr(fileSuffix, '/')) {
        fprintf(stderr, "%s [%s] Error: '/' found in file name, directories MUST be specified using --dir\n",
            timeNow(), threadString);
        errorFlag = 1;
    }

    if (threadCount < 1) {
        fprintf(stderr, "%s [%s] Error: invalid thread-count (%d). Must be 1 or more.\n",
            timeNow(), threadString, threadCount);
        errorFlag = 1;
    }

    if (threadNum < 0 || threadNum > threadCount) {
        fprintf(stderr, "%s [%s] Error: invalud thread-num (%d).  Must be between 1 and thread-count (%d).\n",
            timeNow(), threadString, threadNum, threadCount);
        errorFlag = 1;
    }

    if (threadCount > 1 && useStdout) {
        fprintf(stderr, "%s [%s] Warning: Using thread-count > 1 (multithreading) on the stdout will not produce contiguous primes.\n"
        "%s [%s] Warning: Recommend using --file or manually dividing threads by range (-g and -G).\n", timeNow(), threadString, timeNow(), threadString);
    }

    if (errorFlag) exit(1);

    fileCount = optind - argC;
    files = argV + optind;
}


