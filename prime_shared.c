#define  _XOPEN_SOURCE
#include <time.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <stdarg.h>
#include <pthread.h>

#include <sys/file.h>
#include <stdio.h>
#include "prime_shared.h"

// Config parameters
char * startValueString = "0";
char * startValueScale = "1";
char * endValueString = "1";
char * endValueScale = "1000000000";
char * chunkSizeString = "1";
char * chunkSizeScale = "1000000000";

int threadCount = 1;
int singleThread = 0;
pthread_key_t threadNumKey;

char * fileDir = ".";
char * filePrefix = "prime.";
char * fileSuffix = ".txt";
char * fileInfix = "-";
int useStdout = 1;
int singleFile = 1;
int fileType = FILE_TYPE_TEXT;

int silent = 0;
int verbose = 0;

// For for file based initialisation
char * * files;
int fileCount;



void stdLog(char * str, ...) {
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



void exitError(int num, int errorNumber, char * str, ...) {
	int * threadNumPt = pthread_getspecific(threadNumKey);
	int threadNum = threadNumPt ? *threadNumPt : 0;
	flockfile(stderr);
	fprintf(stderr, "%s [%02d] Error: ", timeNow(), threadNum);
    va_list ap;
    va_start(ap, str);
    vfprintf(stderr, str, ap);
    va_end(ap);
	fprintf(stderr, "\n");
	if (errorNumber) fprintf(stderr, "%s [%02d] Error: %s\n", timeNow(), threadNum,  strerror(errorNumber));
	funlockfile(stderr);
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



void * mallocSafe(size_t bytes) {
	void * result = malloc(bytes);
	if (!result) exitError(255, errno, "Could not alocate to %zd bytes", bytes);
	return result;
}



void * reallocSafe(void * existing, size_t bytes) {
	void * result = realloc(existing, bytes);
	if (!result) exitError(255, errno, "Could not realocate to %zd bytes", bytes);
	return result;
}



void printUsage() {
	fprintf(stderr,
            "Usage: \n"
            "  %s <options> [initialisation file]\n"
            "\n"
            "Start / end options:"
            "  -o --start               start value in units\n"
			"  -O --end                 end value in units\n"
			"  -k --start-thousand      start value in thousands\n"
			"  -K --end-thousand        end value in thousands\n"
			"  -m --start-million       start value in millions\n"
			"  -M -end-million          end value in millions\n"
			"  -g --start-billion       start value in billions\n"
			"  -G --end-billion         end value in billions\n"
			"  -t --start-trillion      start value in trillions\n"
			"  -T --end-trillion        end value in trillions\n"
			"\n"
			"Output options:\n"
			"  -a --text-out            Write primes in text (ASKII new line delimited)\n"
			"  -b --binary-out          Write files in system dependent binary format\n"
			"                           Use -bf for generating an initialization file\n"
			"  -B --compressed-out      Write compressed binary output\n"
			"  -d --dir                 Specify the directory to write to\n"
			"  -f --single-file         Write to a new file instead of stdout\n"
			"  -F --multi-file          Write to one file per chunk instead of stdout\n"
			"     --file-prefix         Specify file name before first number\n"
			"     --file-infix          Specify file name between first and last number\n"
			"     --file-suffix         Specify file name after last number\n"
			"\n"
			"General processing options:"
			"  -c --chunk-million       size of chunks to process in millions\n"
			"                           (affects file size when using -F)\n"
			"  -C --chunk-billion       size of chunks to process in billions\n"
			"                           (affects file size when using -F)\n"
			"  -x --thread-count        Specify the number of threads to use (default 1)\n"
			"\n"
			"Debug & logging options:\n"
			"  -s -q --silent --quet     Disable progess output\n"
			"  -v --verbose              Verbose output, meaningless with -s or -q\n"
			"\n"
			"Other:\n"
			"-h --help                    Show this help");
	exit(0);
}



void parseArgs(int argC, char ** argV) {
	pthread_key_create(&threadNumKey, NULL);

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
                {"file-prefix", required_argument, 0, 1000},
                {"file-suffix", required_argument, 0, 1001},
                {"file-infix", required_argument, 0, 1002},
                {"dir", required_argument, 0, 'd'},
                {"thread-count", required_argument, 0, 'x'},
                {"silent", no_argument, 0, 's'},
				{"quiet", no_argument, 0 , 'q'},
                {"verbose", no_argument, 0, 'v'},
                {"single-file", no_argument, 0, 'f'},
				{"multi-file", no_argument, 0, 'F'},
				{"text-out", no_argument, 0, 'a'},
				{"binary-out", no_argument, 0, 'b'},
				{"help", no_argument, 0, 'h'},
                {0, 0, 0, 0}
            };
    static char * shortOptions ="O:o:K:k:M:m:G:g:T:t:C:c:p:x:abdqsvfFh";

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
            case 'd': fileDir = optarg; break;
            case 'x': threadingNumber = 'x'; break;
            case 'q':
			case 's': silent = 1; verbose = 0; break;
            case 'v': verbose = !silent; break;
            case 'f': useStdout = 0; singleFile = 1; break;
			case 'F': useStdout = 0; singleFile = 0; break;
			case 'a': fileType = FILE_TYPE_TEXT; break;
			case 'b': fileType = FILE_TYPE_SYSTEM_BINARY; break;
			case 'h': printUsage(); break;
            case '?':
                if (optopt) {
                    exitError(1,0,"invalid option -%c", optopt);
                }
                else {
                    exitError(1,0,"invalid option --%s", argV[optind-1]);
                }
                break;
        }

        if (threadingNumber) {
            long value;
            char * endptr;
            value = strtol(optarg, &endptr, 10);
            if (*endptr || value <= 0 || value > 1000) {
				exitError(1,0, "threading number %s is invalud. Must be between 1 and 1000", optarg);
            }
            if (threadingNumber == 'x') {
                threadCount = value;
            }
        }
    }

    // We default to multithreading with 1 thread...
    // This is more normally known as single threading so
    // lets not waste time with the multithreading functions.
    if (threadCount == 1) singleThread = 1;

    int errorFlag = 0;

    if (strchr(filePrefix, '/') || strchr(fileInfix, '/') || strchr(fileSuffix, '/')) {
        exitError(1,0,"'/' found in file name, directories MUST be specified using --dir");
    }

    if (threadCount < 1) {
        exitError(1,0,"invalid thread-count (%d). Must be 1 or more.", threadCount);
    }

    fileCount = optind - argC;
    files = argV + optind;
}


