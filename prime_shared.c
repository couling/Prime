#include "prime_shared.h"

#define  _XOPEN_SOURCE
#include <time.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <stdarg.h>
#include <pthread.h>
#include <unistd.h>

#include <sys/file.h>
#include <sys/stat.h>

#include "shared.h"

// Config parameters

Prime startValue;
Prime endValue;
Prime chunkSize;
Prime lowPrimeMax;

int threadCount;

static char * defaultFileNames [] = {
    "prime.%18e0o-%18e0O.",
    "prime.%15e3oK-%15e3OK.",
    "prime.%12e6oM-%12e6OM.",
    "prime.%9e9oG-%9e9OG."
};

int allowClobber;
char * dirName;
char * fileName;
char * outputProcessor;
char * initFileName;
int useStdout;
int singleFile;
int fileType = FILE_TYPE_TEXT;

char ** inputFiles;
int inputFileCount;

static ChildProcess * outputProcessors;

static void openFilesnprintf(char * base, char ** target, int bufferSize, const char * str, ...) {
    int allowableSize = bufferSize - (base - *target);
    va_list ap;
    va_start(ap, str);
    int bytesWritten = vsnprintf(*target, allowableSize, str, ap);
    va_end(ap);
    if (bytesWritten < 0 || bytesWritten == allowableSize) *target = base + bufferSize;
    else *target += bytesWritten - 1;
}



#define DEFAULT_MULTIPLIER   1
#define DEFAULT_LEFT_PADDING 0

char * formatFileNamePart(char * formattedFileName, int bufferSize, const char * source, Prime from, Prime to) {
    time_t rawtime;
    struct tm * timeinfo;
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    
    // Evaluate the file name
    char * writePosition = formattedFileName;
    const char * readPosition = source;
    Prime multiplyer;
    prime_set_num(multiplyer, DEFAULT_MULTIPLIER);
    int leftPadding = DEFAULT_LEFT_PADDING;
    while(*readPosition != 0 && writePosition < formattedFileName + bufferSize) {
        switch (*readPosition) {
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
                    case 'e':
                    case 'E':
                        if (*readPosition == 'e' || *readPosition == 'E') {
                            readPosition++;
                            if (*readPosition < '0' || *readPosition > '9') {
                                exitError(1,0,"Invalid file name, expected 0 to 9 found: %c", *readPosition);
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
                            exitError(1,0,"Invalid file name, expected o or O found: %c", *readPosition);
                        }
                        PrimeString s;
                        Prime p;
                        prime_cp(p, *readPosition == 'o' ? from : to);
                        prime_div_prime(p, p, multiplyer);
                        prime_to_str(s, p);
                        leftPadding -= strlen(s);
                        if (leftPadding > 0) {
                            
                            openFilesnprintf(formattedFileName, &writePosition, bufferSize, "%.*s%s", leftPadding,
                            "0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000",
                            s);
                            
                        }
                        else {
                            openFilesnprintf(formattedFileName, &writePosition, bufferSize, "%s", s);
                        }
                        prime_set_num(multiplyer, DEFAULT_MULTIPLIER);
                        leftPadding = DEFAULT_LEFT_PADDING;
                        break;
                    }


                    case 't': {
                        static char t[100];
                        strftime(t, 100, "%H-%M-%S", timeinfo);
                        openFilesnprintf(formattedFileName, &writePosition, bufferSize, "%s", t);
                        break;
                    }
 
                    case 'd': {
                        static char t[100];
                        strftime(t, 100, "%Y-%b-%d", timeinfo);
                        openFilesnprintf(formattedFileName, &writePosition, bufferSize, "%s", t);
                        break;
                    }

                    case 'x': {
                        int * threadNumPt = pthread_getspecific(threadNumKey);
                        int threadNum = threadNumPt ? *threadNumPt : 0;
                        openFilesnprintf(formattedFileName, &writePosition, bufferSize, "%d", threadNum);
                        break;
                    }

                    default:
                        exitError(1, 0, "Unrecognised character %c", *readPosition);
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
    if (writePosition < formattedFileName + bufferSize) {
        *writePosition = '\0';
    } else {
        exitError(1, 0, "File name format too long: %s", fileName);
    }

    return writePosition;
}



char * formatFileName( char * formattedFileName, int bufferSize, Prime from, Prime to) {
    char * writeBuffer = formattedFileName;
    if (fileName[0] != '/') {
        writeBuffer = formatFileNamePart(writeBuffer, bufferSize, dirName, from, to);
        bufferSize -= writeBuffer - formattedFileName;
        if (writeBuffer != formattedFileName && *(writeBuffer-1) != '/') {
            writeBuffer = formatFileNamePart(writeBuffer, bufferSize, "/", from, to);
            bufferSize -= writeBuffer - formattedFileName;
        }
    }
    return formatFileNamePart(writeBuffer, bufferSize, fileName, from, to);
}



int openFileForPrime(Prime from, Prime to) {
    int file;
    if (useStdout) {
        file = STDOUT_FILENO;
    }
    else {
        // Evaluate the file name
        char formattedFileName[FILENAME_MAX];
        formatFileName(formattedFileName, FILENAME_MAX, from, to);

        // Create necessary directories
        mkdirs(formattedFileName);

        // Create the file
        if (!silent) stdLog("Starting new prime file: %s", formattedFileName);
        file = open(formattedFileName, O_WRONLY | O_CREAT | ( allowClobber ? O_TRUNC : O_EXCL ), 0644);
        if (file == -1) exitError(2, errno, "Could not create new file: %s", formattedFileName);
    }

    if (outputProcessor) {
        int threadNum;
        if (singleFile) {
            threadNum = 0;
        }
        else {
            int * threadNumPt = pthread_getspecific(threadNumKey);
            threadNum = threadNumPt ? (*threadNumPt)-1 : 0;
        }
        if (outputProcessors[threadNum].processID != 0)
            exitError(1,0,"Thread Attempting to open more than one output processor simaltainiously.  This is not allowed.");

        execPipeProcess(&(outputProcessors[threadNum]), outputProcessor, -1, file);

        file = outputProcessors[threadNum].stdin;
    }
    return file;
}



void closeFileForPrime(int fd) {
    if (outputProcessor) {
        int threadNum;
        if (singleFile) {
            threadNum = 0;
        }
        else {
            int * threadNumPt = pthread_getspecific(threadNumKey);
            threadNum = threadNumPt ? (*threadNumPt)-1 : 0;
        }
        if (fd != outputProcessors[threadNum].stdin)
            exitError(1,0,"Thread Attempting to close file that doesn't belong to it");
        closeAndWait(outputProcessors+threadNum);
    }
    else {
        if (!close(fd)) exitError(1,errno, "Could not close output file, contents may have been truncated");
    }
}



static void setFN(int level) {
    // Find the level
    int i = 4;
    while (i > 0 && fileName != defaultFileNames[i]) i--;
    // Only change if we are changing to a lower level
    // existing level will be -1 if set to a custom string.
    if (level < i) fileName = defaultFileNames[level];
}

#define QUOTE(name) #name
#define STR_VALUE(arg) QUOTE(arg)
char * getVersion() {
    static char buffer[192];
    int x = snprintf(buffer, 192,
    STR_VALUE(PRIME_PROGRAM_NAME) " (" STR_VALUE(PRIME_PROGRAM_VERSION) ")"
    "\nCopyright (C) 2013 Philip Couling"
    "\nArchitechture: " STR_VALUE(PRIME_ARCHITECTURE)
    "\nPrime size: %zd bit"
    "\nPrime subdivision: %zd elements of %zd bits"
    "\nBuilt: " __DATE__ " " __TIME__ "\n", sizeof(Prime) * 8, PRIME_LIMB_COUNT, PRIME_LIMB_SIZE * 8);
    return buffer;
}



static void printUsage(int argC, char ** argV) {
    fprintf(stdout,
            "Usage: \n"
            "  %s <options> [initialisation file]\n"
            "\n"
            "Start / end options:\n"
            "  -s --start               start value\n"
            "                           suffix this with k,m,g,t to multiply by\n"
            "                           one thousand, million, billion or trillion\n"
            "  -e --end                 end value\n"
            "                           suffix this with k,m,g,t to multiply by\n"
            "                           one thousand, million, billion or trillion\n"
            "\n"
            "Output options:\n"
            "  -a --text-out            Write primes in text (ASKII new line delimited)\n"
            "  -b --binary-out          Write files in system dependent binary format\n"
            "                           Use -bf for generating an initialization file\n"
            "  -B --compressed-out      Write compressed binary output - the smallest file possible\n"
            "                           The format of this file is unique to prime programs\n"
            "                           It is NOT gzip or bzip2 format!\n"
            "  -d --directory           Specify the output directory, will be ignored if file name\n"
            "                           starts with /\n"
            "  -n --file-name           Specify the file name as a pattern\n"
            "                           Eg: prime.%%9e9oG-%%9e9OG.txt\n"
            "  -f --single-file         Write to a new file\n"
            "  -F --multi-file          Write to one file per chunk\n"
            "  -p --use-stdout          Write to the stdout, will not create files\n"
            "  -k --clobber             Allow overwriting of existing files\n"
            "  -I --create-init-file    Equivalent to -bfs 3 -n init-%%9e9OG.dat\n"
            "  -P --post-process        Pipes all content through a command (eg: gzip)\n"
            "                           This is specified as a single string containing\n"
			"                           both the command and its arguments\n"
            "\n"
            "General processing options:\n"
            "  -c --chunk-size          size of chunks to process\n"
            "                           suffix this with k,m,g,t to multiply by\n"
            "                           one thousand, million, billion or trillion\n"
            "                           (affects file size when using -F)\n"
            "  -l --low-prime-max       The maximum value for low primes.\n"
            "                           This can not be set above 23\n"
            "  -x --threads             Specify the number of threads to use (default 1)\n"
            "  -i --init-file           Specify an initialisation file generated with -b previously\n"
            "\n"
#ifndef STRIP_LOGGING
            "Debug & logging options:\n"
            "  -q --quiet                Same as -s\n"
            "  -v --verbose              Verbose output, meaningless with -s or -q\n"
            "\n"
#endif
            "Other:\n"
            "  -h --help                 Show this help and exit\n"
            "  -V --version              Print the program version and exit\n", argV[0]);
}



static void stringToSize(Prime * target, char * size) {
    size_t stringSize = strlen(size);
    char multiplyerChar;
    char * multiplyerString;
    if (stringSize == 0) exitError(1,0,"No value given for number");
    multiplyerChar = size[stringSize-1];
    if (multiplyerChar < '0' || multiplyerChar > '9') {
        switch (multiplyerChar) {
            case 'k':
            case 'K':
                multiplyerString = "1000";
                setFN(1);
                break;
            case 'm':
            case 'M':
                setFN(2);
                multiplyerString = "1000000";
                break;
            case 'g':
            case 'G':
                setFN(3);
                multiplyerString = "1000000000";
                break;
            case 't':
            case 'T':
                setFN(3);
                multiplyerString = "1000000000000";
                break;
            default:
                exitError(1,0,"Unrecognised value %s", size);
        }
        size[stringSize-1] = '\0';
    }
    else {
        setFN(0);
        multiplyerString = "1";
    }
    str_to_prime(*target, size);
    Prime multiplyer;
    str_to_prime(multiplyer, multiplyerString);
    prime_mul_prime(*target,*target,multiplyer);
}



void parseArgs(int argC, char ** argV) {
    threadCount = 1;

    useStdout  = 0;
    singleFile = 0;
    fileType   = FILE_TYPE_TEXT;

#ifndef STRIP_LOGGING
    silent  = 0;
    verbose = 0;
#endif

    prime_set_num(startValue, 0);
    prime_set_num(endValue,   1000000000);
    prime_set_num(chunkSize,  1000000000);
    prime_set_num(lowPrimeMax,19);
    

    dirName      = "";
    fileName     = defaultFileNames[3];
    allowClobber = 0;
    initFileName = NULL;

    
    static struct option longOptions[] = {
            { "start", required_argument, 0, 's' },
            { "end", required_argument, 0, 'e' },
            { "chunk-size", required_argument, 0, 'c' },
            { "low-prime-max", required_argument, 0 , 'l'},
            { "threads", required_argument, 0, 'x' },
            { "directory", required_argument, 0, 'd'},
            { "file-name", required_argument, 0, 'n'},
            { "init-file", required_argument, 0, 'i'},
            { "post-process", required_argument, 0, 'P'},
#ifndef STRIP_LOGGING
            { "quiet", no_argument, 0, 'q' },
            { "verbose", no_argument, 0, 'v' },
#endif
            { "single-file", no_argument, 0, 'f' },
            { "multi-file", no_argument, 0, 'F' },
            { "use-stdout", no_argument, 0, 'p'},
            { "text-out", no_argument, 0, 'a' },
            { "binary-out", no_argument, 0, 'b' },
            { "compressed-out", no_argument, 0, 'B' },
            { "clobber", no_argument, 0, 'k'},
            { "create-init-file", no_argument, 0, 'I'},
            { "help", no_argument, 0, 'h' },
            { "version", no_argument, 0, 'V'},
            { 0, 0, 0, 0 }
    };

#ifndef STRIP_LOGGING
    static char * shortOptions = "s:e:c:d:n:i:x:P:qvfFpabBhkIV";
#else
    static char * shortOptions = "s:e:c:d:n:i:x:P:fFpabBhkIV";
#endif
    int givenOption;
    // do not allow getopt_long to print an error to stdout if an invalid option is found
    opterr = 0;
    while ((givenOption = getopt_long(argC, argV, shortOptions, longOptions, NULL)) != -1) {
        switch (givenOption) {
        case 's': stringToSize(&startValue, optarg);              break;
        case 'e': stringToSize(&endValue, optarg);                break;
        case 'c': stringToSize(&chunkSize, optarg);               break;
        case 'l': {
            long value;
            char * endptr;
            value = strtol(optarg, &endptr, 10);
            if (*endptr || value < 0 || value > 23) {
                exitError(1, 0,
                        "max low prime %s is invalid. Must be between 1 and 21",
                        optarg);
            }
            prime_set_num(lowPrimeMax, value);
            break;
        }

#ifndef STRIP_LOGGING
        case 'q': silent = 1;                 verbose = 0;        break;
        case 'v': verbose = !silent;                              break;
#endif
        case 'f': useStdout = 0;              singleFile = 1;     break;
        case 'F': useStdout = 0;              singleFile = 0;     break;
        case 'p': useStdout = 1;              singleFile = 1;     break;
        case 'P': outputProcessor = optarg;                       break;
        case 'a': fileType = FILE_TYPE_TEXT;                      break;
        case 'b': fileType = FILE_TYPE_SYSTEM_BINARY;             break;
        case 'B': fileType = FILE_TYPE_COMPRESSED_BINARY;         break;
        case 'd': dirName  = optarg;                              break;
        case 'n': fileName = optarg;                              break;
        case 'i': initFileName = optarg;                          break;
        case 'I': fileType = FILE_TYPE_SYSTEM_BINARY; 
                  useStdout = 0; singleFile = 1; 
                  prime_set_num(startValue, 3);                   
                  fileName = "init-%9e9OG.dat";                   break;
        case 'h': printUsage(argC, argV);              exit(0);   break;
        case 'V': fprintf(stderr, "%s", getVersion()); exit(0);   break;
        case 'k': allowClobber = 1;                               break;
        case 'x': {
            long value;
            char * endptr;
            value = strtol(optarg, &endptr, 10);
            if (*endptr || value <= 0 || value > 1000) {
                exitError(1, 0,
                        "threading number %s is invalid. Must be between 1 and 1000",
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


    if (fileName == defaultFileNames[0] 
    || fileName == defaultFileNames[1] 
    || fileName == defaultFileNames[2] 
    || fileName == defaultFileNames[3]) {
        static char tmpFileName[50];
        strcpy(tmpFileName, fileName);
        if (fileType == FILE_TYPE_TEXT) strcat(tmpFileName,"txt");
        else if (fileType == FILE_TYPE_COMPRESSED_BINARY) strcat(tmpFileName,"primefile");
        else strcat(tmpFileName,"dat");
        fileName = tmpFileName;
    }

    inputFileCount = argC - optind;
    inputFiles = argV + optind;


    if (threadCount < 1) exitError(1, 0, "invalid thread-count (%d). Must be 1 or more.", threadCount);

    if (outputProcessor) {
        if (singleFile) {
            outputProcessors = mallocSafe(sizeof(ChildProcess));
            memset(outputProcessors, 0, sizeof(ChildProcess));
        }
        else {
            outputProcessors = mallocSafe(sizeof(ChildProcess) * threadCount);
            memset(outputProcessors, 0, sizeof(ChildProcess) * threadCount);
        }
    }
}

