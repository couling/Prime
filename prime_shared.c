#define  _XOPEN_SOURCE
#include <time.h>

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <stdarg.h>
#include <pthread.h>

#include <sys/file.h>
#include <sys/stat.h>

#include "prime_shared.h"

// Config parameters

Prime startValue;
Prime endValue;
Prime chunkSize;

int threadCount;

char * fileName;
char * initFileName;
int useStdout;
int singleFile;
int fileType = FILE_TYPE_TEXT;

int silent;
int verbose;


// For thread local storage
pthread_key_t threadNumKey;



static char * defaultFileNames [] = {
    "prime.%18x0o-%18x0O.txt",
    "prime.%15x3ok-%15x3OK.txt",
    "prime.%12x6om-%12x6OM.txt",
    "prime.%9x9og-%9x9OG.txt"
};



static char * timeNow() {
	time_t rawtime;
	struct tm * timeinfo;
	time(&rawtime);
	timeinfo = localtime(&rawtime);
	static char t[100];
	strftime(t, 100, "%a %b %d %H:%M:%S %Y", timeinfo);
	return t;
}



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
	if (errorNumber)
		fprintf(stderr, "%s [%02d] Error: %s\n", timeNow(), threadNum, strerror(errorNumber));
	funlockfile(stderr);
	exit(num);
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



static void openFilesnprintf(char * base, char ** target, const char * str, ...) {
	int allowableSize = FILENAME_MAX - (base - *target);
	va_list ap;
	va_start(ap, str);
	int bytesWritten = vsnprintf(*target, allowableSize, str, ap);
	va_end(ap);
	if (bytesWritten < 0 || bytesWritten == allowableSize) *target = base + FILENAME_MAX;
	else *target += bytesWritten - 1;
}



#define DEFAULT_MULTIPLIER   1
#define DEFAULT_LEFT_PADDING 0


FILE * openFileForPrime(Prime from, Prime to) {
    // Evaluate the file name
	char formattedFileName [FILENAME_MAX];
	char * writePosition = formattedFileName;
	char * readPosition = fileName;
	Prime multiplyer;
	prime_set_num(multiplyer, DEFAULT_MULTIPLIER);
	int leftPadding = DEFAULT_LEFT_PADDING;
	while(*readPosition != 0 && writePosition < formattedFileName + FILENAME_MAX) {
		switch (*readPosition) {
			case '/': 
				*writePosition = '\0';
				if (!mkdir(formattedFileName, S_IRWXU)) {
					stdLog("Created directory for output: %s", formattedFileName);
				}
				*writePosition = '/';
				break;
			
			case '%':
				readPosition++;
				switch (*readPosition) {
					case '%':
						*writePosition = '%';
						break;
			
					case '0':
					case '1':
					case '2':
					case '3':
					case '4':
					case '5':
					case '6':
					case '7':
					case '8':
					case '9':
						leftPadding = 0;
						do leftPadding = (leftPadding * 10) + (*(readPosition++) - '0' );
						while (*readPosition >= '0' && *readPosition <= '9');
					case 'x':
					case 'X':
						if (*readPosition == 'x' || *readPosition == 'X') {
							readPosition++;
							if (*readPosition < '0' || *readPosition > '9') {
								exitError(1,0,"Invalid filename, expected 0 to 9 found: %c", *readPosition);
							}
							int multiplyerInt = 0;
							do multiplyerInt = (multiplyerInt * 10) + (*(readPosition++) - '0' );
							while (*readPosition >= '0' && *readPosition <= '9');
							prime_set_num(multiplyer, 1);
							for (int i=0; i < multiplyerInt; i++) {
								prime_mul_num(multiplyer, multiplyer, 10);
							}
						}
					case 'o':
					case 'O': {
						if (*readPosition != 'o' && *readPosition != 'O') {
							exitError(1,0,"Invalid filename, expected o or O found: %c", *readPosition);
						}
						PrimeString s;
						Prime p;
						prime_cp(p, *readPosition == 'o' ? from : to);
						prime_div_prime(p, p, multiplyer);
						prime_to_str(s, p);
						leftPadding -= strlen(s);
						if (leftPadding > 0) {
							
							openFilesnprintf(formattedFileName, &writePosition, "%.*s%s", leftPadding,
							"0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000",
							s);
							
						}
						else {
							openFilesnprintf(formattedFileName, &writePosition, "%s", s);
						}
						prime_set_num(multiplyer, DEFAULT_MULTIPLIER);
						leftPadding = DEFAULT_LEFT_PADDING;
						break;
					}

					default:
						exitError(1, 0, "Unrecognized character %c", *readPosition);
						break;
				}
				break;

			default:
				*writePosition = *readPosition;
				break;
		}

		readPosition = readPosition + 1;
		writePosition = writePosition + 1;
	}
	if (writePosition < formattedFileName + FILENAME_MAX) {
		*writePosition = '\0';
	} else {
		exitError(1, 0, "Filename format too long: %s", fileName);
	}

	if (!silent)
		stdLog("Starting new prime file: %s", formattedFileName);
	FILE * file = fopen(formattedFileName, "wx");
	if (!file)
		exitError(2, errno, "Could not create new file: %s", formattedFileName);

}



static void setFN(int level) {
	// Find the level
	int i = 4;
	while (i > 0 && fileName != defaultFileNames[i]) i--;
	// Only change if we are changing to a lower level
	// existing level will be -1 if set to a custom string.
	if (level < i) fileName = defaultFileNames[level];
}



static void printUsage(int argC, char ** argV) {
	fprintf(stderr,
			"Usage: \n"
			"  %s <options> [initialisation file]\n"
			"\n"
			"Start / end options:\n"
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
			"  -B --compressed-out      Write compressed binary output - the smalles file possible\n"
			"                           The format of this file is unique to prime programs\n"
			"                           It is NOT gzip or bzip2 format!\n"
			"  -n --file-name           Specify the file name as a pattern\n"
			"                           Eg: prime.%%9x9og-%%9x9OG.txt\n"
			"  -f --single-file         Write to a new file instead of stdout\n"
			"  -F --multi-file          Write to one file per chunk instead of stdout\n"
			"\n"
			"General processing options:"
			"  -c --chunk-million       size of chunks to process in millions\n"
			"                           (affects file size when using -F)\n"
			"  -C --chunk-billion       size of chunks to process in billions\n"
			"                           (affects file size when using -F)\n"
			"  -x --thread-count        Specify the number of threads to use (default 1)\n"
			"  -i --init-file           Specifiy an initialisation file generated with -b previously\n"
			"\n"
			"Debug & logging options:\n"
			"  -s --silent               Disable progess output\n"
			"  -q --quiet                Same as -s\n"
			"  -v --verbose              Verbose output, meaningless with -s or -q\n"
			"\n"
			"Other:\n"
			"  -h --help                 Show this help\n", argV[0]);
}



void parseArgs(int argC, char ** argV) {
	threadCount = 1;

	useStdout = 0;
	singleFile = 0;
	fileType = FILE_TYPE_TEXT;

	silent = 0;
	verbose = 0;


	char * startValueString = "0";
	char * startValueScale = "1";
	char * endValueString = "1";
	char * endValueScale = "1000000000";
	char * chunkSizeString = "1";
	char * chunkSizeScale = "1000000000";	
	

	fileName = defaultFileNames[3];
	initFileName = NULL;

	pthread_key_create(&threadNumKey, NULL);

	static struct option longOptions[] = {
			{ "start", required_argument, 0, 'o' },
			{ "end", required_argument, 0, 'O' },
			{ "start-thousand", required_argument, 0, 'k' },
			{ "end-thousand", required_argument, 0, 'K' },
			{ "start-million", required_argument, 0, 'm' },
			{ "end-million", required_argument, 0, 'M' },
			{ "start-billion", required_argument, 0, 'g' },
			{ "end-billion", required_argument, 0, 'G' },
			{ "start-trillion", required_argument, 0, 't' },
			{ "end-trillion", required_argument, 0, 'T' },
			{ "chunk-million", required_argument, 0, 'c' },
			{ "chunk-billion", required_argument, 0, 'C' },
			{ "thread-count", required_argument, 0, 'x' },
			{ "file-name", required_argument, 0, 'n'},
			{ "init-file", required_argument, 0, 'i'},
			{ "silent", no_argument, 0, 's' },
			{ "quiet", no_argument, 0, 'q' },
			{ "verbose", no_argument, 0, 'v' },
			{ "single-file", no_argument, 0, 'f' },
			{ "multi-file", no_argument, 0, 'F' },
			{ "use-stdout", no_argument, 0, 'p'},
			{ "text-out", no_argument, 0, 'a' },
			{ "binary-out", no_argument, 0, 'b' },
			{ "compressed-out", no_argument, 0, 'B' },
			{ "help", no_argument, 0, 'h' },
			{ 0, 0, 0, 0 }
	};
	static char * shortOptions = "O:o:K:k:M:m:G:g:T:t:C:c:n:i:x:sqvfFpabBh";

	int givenOption;
	// do not allow getopt_long to print an error to stdout if an invalid option is found
	opterr = 0;
	while ((givenOption = getopt_long(argC, argV, shortOptions, longOptions, NULL)) != -1) {
		switch (givenOption) {
		case 'o': startValueString = optarg;  startValueScale = "1";              setFN(0); break;
		case 'O': endValueString   = optarg;  endValueScale   = "1";              setFN(0); break;
		case 'k': startValueString = optarg;  startValueScale = "1000";           setFN(1); break;
		case 'K': endValueString   = optarg;  endValueScale   = "1000";           setFN(1); break;
		case 'm': startValueString = optarg;  startValueScale = "1000000";        setFN(2); break;
		case 'M': endValueString   = optarg;  endValueScale   = "1000000";        setFN(2); break;
		case 'g': startValueString = optarg;  startValueScale = "1000000000";     setFN(3); break;
		case 'G': endValueString   = optarg;  endValueScale   = "1000000000";     setFN(3); break;
		case 't': startValueString = optarg;  startValueScale = "1000000000000";  setFN(3); break;
		case 'T': endValueString   = optarg;  endValueScale   = "1000000000000";  setFN(3); break;
		case 'c': chunkSizeString  = optarg;  chunkSizeScale  = "1000000";        setFN(2); break;
		case 'C': chunkSizeString  = optarg;  chunkSizeScale  = "1000000000";     setFN(3); break;
		case 'q':
		case 's': silent = 1;                 verbose = 0;        break;
		case 'v': verbose = !silent;                              break;
		case 'f': useStdout = 0;              singleFile = 1;     break;
		case 'F': useStdout = 0;              singleFile = 0;     break;
		case 'p': useStdout = 0;                                  break;
		case 'a': fileType = FILE_TYPE_TEXT;                      break;
		case 'b': fileType = FILE_TYPE_SYSTEM_BINARY;             break;
		case 'B': fileType = FILE_TYPE_COMPRESSED_BINARY;         break;
		case 'n': fileName = optarg;                              break;
		case 'i': initFileName = optarg;                          break;
		case 'h': printUsage(argC, argV);     exit(0);            break;
		case 'x': {
            long value;
            char * endptr;
            value = strtol(optarg, &endptr, 10);
            if (*endptr || value <= 0 || value > 1000) {
                exitError(1, 0,
                        "threading number %s is invalud. Must be between 1 and 1000",
                        optarg);
            }
            threadCount = value;
			break;
		}
            		
		case '?':
		default:
			if (optopt) {
				exitError(1, 0, "invalid option -%c use %s -h for usage", optopt, argV[0]);
			} else {
				exitError(1, 0, "invalid option %s use %s -h for usage", argV[optind - 1], argV [0]);
			}
			break;
		}
	}

	if (optind != argC) {
		exitError(1 , 0, "invalid option %s", argV[optind]);
	}

    // Set the start and finish values
	Prime scale;
	str_to_prime(scale, startValueScale);
	str_to_prime(startValue, startValueString);
	prime_mul_prime(startValue, startValue, scale);
	str_to_prime(scale, endValueScale);
	str_to_prime(endValue, endValueString);
	prime_mul_prime(endValue, endValue, scale);
	str_to_prime(scale, chunkSizeScale);
	str_to_prime(chunkSize, chunkSizeString);
	prime_mul_prime(chunkSize, chunkSize, scale);

	if (threadCount < 1) exitError(1, 0, "invalid thread-count (%d). Must be 1 or more.", threadCount);
}
