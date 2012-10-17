#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <getopt.h>
#include <string.h>
#include <errno.h>

//#define VERBOSE_DEBUG

#define ALLOC_UNIT 1024

// Config parameters
char * * files;
int fileCount;
long long startValue = 0;
long long endValue = 1000000000ll;
long long chunkSize = 1000000000ll;

char * fileDir = ".";
char * filePrefix = "prime.";
char * fileSuffix = ".txt";
char * fileInfix = "-";
char fileLabelType = 'g';
int useStdout = 1;

size_t primeCount;                 // Number of primes stored
long long * primes;                // The array of primes

unsigned char removeMask[] = {0xFF, 0xFE, 0xFF, 0xFD, 0xFF, 0xFB, 0xFF, 0xF7, 0xFF, 0xEF, 0xFF, 0xDF, 0xFF, 0xBF, 0xFF, 0x7F};
unsigned char checkMask[] =  {0x00, 0x01, 0x00, 0x02, 0x00, 0x04, 0x00, 0x08, 0x00, 0x10, 0x00, 0x20, 0x00, 0x40, 0x00, 0x80};



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



void applyPrime(long long prime, long long offset, unsigned char * map, size_t mapSize) {
    long long value;
    if (prime < offset) {
        value = prime - ((offset - 1) % prime) - 1;
        if (!(value & 1)) value += prime;
    }
    else {
        value = (prime * prime) - offset;
    }
    long long stepSize = prime << 1;
    long long applyTo = ((long long) mapSize) << 4;
    while (value < applyTo) {
        #ifdef VERBOSE_DEBUG
        fprintf(stderr,"%s prime = %lld, value = %lld, map[%lld] = %.2X, "
            "removeMask[%d] = %.2X, result = %.2X, offset = %lld\n", 
            timeNow(), prime, value, value >> 4, (int) map[value >> 4], (int) value & 0x0F, 
            (int) removeMask[value & 0x0F], (int)(map[value >> 4] & removeMask[value & 0x0F]),
            offset);
        #endif
        map[value >> 4] &= removeMask[value & 0x0F];
        value += stepSize;
    }
}



void initializeSelf() {
    fprintf(stderr, "%s Running Self initialisation for %lld (inc) to %lld (ex)\n", 
        timeNow(), startValue, endValue);

    size_t primesAllocated = ALLOC_UNIT;
    primes = mallocSafe(primesAllocated * sizeof(long long));
    size_t range = ALLOC_UNIT;
    unsigned char * bitmap = mallocSafe(range * sizeof(unsigned char));
    memset(bitmap, 0xFF, range);

    long long value=3ll;
    primeCount = 0;

    while (1) {
        long long maxSize = (((long long) range) << 4);
        while (value < maxSize) {
            if (bitmap[value >> 4] & checkMask[value & 0x0F]) {
                applyPrime(value, 0, bitmap, range);
                if (primeCount == primesAllocated) {
                    fprintf(stderr,"%s Initialized %zd primes, reached value %lld (ex)\n", 
                        timeNow(), primeCount, value);
                    primesAllocated += ALLOC_UNIT;
                    primes = reallocSafe(primes, primesAllocated * sizeof(long long));
                }
                primes[primeCount] = value;
                ++primeCount;
                if (value * value > endValue) goto Complete;
            }
            #ifdef VERBOSE_DEBUG
            else {
                fprintf(stderr,"%s Ruled out %lld due to bitmap[%d] = %d and checkMask[%d] = %d\n", 
                    timeNow(), value, (int) value >> 4, (int)bitmap[value >> 4], 
                    (int)value & 0x0F, (int)checkMask[value & 0x0F]);
            }
            #endif
            value+=2;
        }
        bitmap = reallocSafe(bitmap, (range + ALLOC_UNIT) * sizeof(unsigned char)); 
        long long offset = ((long long) range) << 4;
        unsigned char * newbitmap = bitmap +  range;
        memset(newbitmap, 0xFF, ALLOC_UNIT * sizeof(unsigned char));
        for (size_t i = 0; i < primeCount; ++i)
            applyPrime(primes[i], offset, newbitmap, ALLOC_UNIT);
        range += ALLOC_UNIT;
    }

    Complete:

    free(bitmap);

    fprintf(stderr,"%s Prime array now full with %zd primes\n", timeNow(), primeCount);
}



void finalSelf() {
    free(primes);
}



void initializeFromFile() {
    fprintf(stderr, "%s Initialization from file has not yet been implemented\n", 
        timeNow(), startValue, endValue);
    exit(1);
}




void finalFile() {
    // Can never get here as files aren't implemented
}



void printValue(FILE * file, long long value) {
    if (fprintf(file,"%lld\n",value) < 0) {
        int lerrno = errno;
        fprintf(stderr, "%s Error: Failed to write prime number (%lld)\n",
            timeNow(), value);
        exitError(2, lerrno);
    }
}


void process(long long from, long long to, FILE * file) {
    fprintf(stderr, "%s Running process for %lld (inc) to %lld (ex)\n", 
        timeNow(), from, to);
    if (from <= 2 && to > 2) printValue(file,2ll);

    if (from <= 2) {
        // Process ignores even primes
        // Process doesnt know 1 isnt a prime, so skip it!
        from = 2;
    }
    else {
        // Process expects to aligned to a even number.
        // If it's odd start a number earlier, the even can never be found
        // so it doesn't matter that we start on it.
        if (from & 0x1) --from;
    }
    
    size_t range = (to - from + 0x0F) >> 4;
    fprintf(stderr, "%s Bitmap will contain %zd bytes\n", 
        timeNow(), range);

    unsigned char * bitmap = mallocSafe(range);
    memset(bitmap, 0xFF, range);

    fprintf(stderr, "%s Applying all primes\n", 
        timeNow(), from, to);

    for (size_t i = 0; i < primeCount; ++i) {
        #ifndef VERBOSE_DEBUG
        if (!(i & 0x7FF)) {
        #endif
            fprintf(stderr, "%s Applying primes %02.2f%% (%zd of %zd [%lld])\n", 
                timeNow(), 100 * ((double)i) / ((double)primeCount),i+1, primeCount, primes[i]);
        #ifndef VERBOSE_DEBUG

        }
        #endif
	applyPrime(primes[i], from, bitmap, range);
    }

    size_t endRange = range -1;
    for (size_t i = 0; i < endRange; ++i) {
        if (!(i & 0x3FFFFF)) {
            fprintf(stderr, "%s Scanning for new primes %02.2f%%\n", 
                timeNow(), 100 * ((double) i)/((double) range));
        }
        if (bitmap[i]) {
            for (int j = 1; j < 16; j+=2) {
                if (bitmap[i] & checkMask[j]) {
                    long long value = from + (((long long) i) << 4) + j;
                    #ifdef VERBOSE_DEBUG
                    fprintf(stderr,"%s found prime = %lld, map[%lld] = %.2X, checkMask[%d] = %.2X, "
                        "result = %.2X, from = %lld\n", 
                        timeNow(), value, (long long)i, (int) bitmap[i], (int) j, 
                        (int) checkMask[j], (int)(bitmap[i] & checkMask[j]), from);
                    #endif
                    printValue(file,value);
                }
            }
        }
    }
    fprintf(stderr, "%s Scanning last byte of bitmap for new primes\n", 
        timeNow(), startValue, endValue);
    if (bitmap[endRange]) {
        for (int j = 1; j < 16; j+=2) {
            if (bitmap[endRange] & checkMask[j]) {
                long long value = from + (((long long) endRange) << 4) + j;
                #ifdef VERBOSE_DEBUG
                fprintf(stderr,"%s found prime = %lld, map[%lld] = %.2X, checkMask[%d] = %.2X, "
                    "result = %.2X, from = %lld\n", 
                    timeNow(), value, (long long)endRange, (int) bitmap[endRange], (int) j, 
                    (int) checkMask[j], (int)(bitmap[endRange] & checkMask[j]), from);
                #endif
                if (value < endValue) printValue(file,value);
            }
        }
    }

    fprintf(stderr, "%s All primes have now been discovered between %lld (inc) and %lld (ex)\n",
        timeNow(), from, to);

    free(bitmap);
}



void processToFile(long long from, long long to) {
    if (useStdout) {
        process(from, to, stdout);
    }
    else {
        char fileName[FILENAME_MAX];
        if (fileLabelType = 'g') {
            snprintf(fileName, FILENAME_MAX,"%s/%sg%04lld%sG%04lld%s",
                fileDir,filePrefix,from/1000000000,fileInfix,to/1000000000,fileSuffix);
        }
        else /* (fileLabelType = 'm') */ {
            int fromPrecision = (from % 1000000000 ? 3 : 0);
            int toPrecision = (to % 1000000000 ? 3 : 0);
            snprintf(fileName, FILENAME_MAX,"%s/%sg%04lld%sG%04lld%s",
                fileDir,filePrefix,from/1000000,fileInfix,to/1000000,fileSuffix);
        }
        fprintf(stderr, "%s Starting new prime file: %s\n",
            timeNow(), fileName);
        FILE * file = fopen(fileName, "wx");
        if (!file) exitError(2, errno);
        process(from, to, file);
        if (fclose(file)) {
            int lerrno = errno;
            fprintf(stderr, "%s Error: closing prime file reported error "
                "contents may have been truncated. \n%s Error: filename %s\n",
                timeNow(), timeNow(), fileName);
            exitError(2, lerrno);
        }
        
    }
}



void processAll() {
    long long from = startValue;
    long long to = startValue - (startValue % chunkSize) + chunkSize;
    while (to < endValue) {
        processToFile(from, to);
        from = to;
        to += chunkSize;
    }
    if (from < endValue) {
        processToFile(from, endValue);
    }
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
               {"chunk-million", required_argument, 0, 'c'},
               {"chunk-billion", required_argument, 0, 'C'},
               {"prefix", required_argument, 0, 1000},
               {"suffix", required_argument, 0, 1001},
               {"infix", required_argument, 0, 1002},
               {"dir", required_argument, 0, 1003},
               {"stdout", no_argument, 0, 's'},
               {"file", no_argument, 0, 'f'},
               {0, 0, 0, 0}
             };    
    static char * shortOptions ="O:o:K:k:M:m:G:g:C:c:p:s:Sf";

    int givenOption;
    // do not allow getopt_long to print an error to stdout if an invalid option is found
    opterr = 0;
    while ((givenOption = getopt_long (argC, argV, shortOptions, longOptions, NULL)) != -1) {
        long long scale;
        int number = 0;
        switch (givenOption) {
            case 'o': scale = 1ll; number = 's'; break;
            case 'O': scale = 1ll; number = 'e'; break;
            case 'k': scale = 1000ll; number = 's'; break;
            case 'K': scale = 1000ll; number = 'e'; break;
            case 'm': scale = 1000000ll; number = 's'; break;
            case 'M': scale = 1000000ll; number = 'e'; break;
            case 'g': scale = 1000000000ll; number = 's'; break;
            case 'G': scale = 1000000000ll; number =  'e'; break;
            case 'c': scale = 1000000ll; number = 'c'; fileLabelType = 'm'; break;
            case 'C': scale = 1000000000ll; number = 'c'; fileLabelType = 'g'; break;
            case 1000: filePrefix = optarg; break;
            case 1001: fileSuffix = optarg; break;
            case 1002: fileInfix = optarg; break;
            case 1003: fileDir = optarg; break;
            case 's': useStdout = 1; break;
            case 'f': useStdout = 0; break;
            case '?': 
                if (optopt) {
                    fprintf(stderr, "%s Error: option -%c\n", 
                        timeNow(), optopt);
                }
                else {
                    fprintf(stderr, "%s Error: option %s\n", 
                        timeNow(), argV[optind-1]);
                }
                exit(1);
                break;
        }

        if (number) {
            long long value;

            char * endptr;
            value = strtoll(optarg, &endptr, 10);
            if (*endptr || value < 0) {
                fprintf(stderr, "%s Error: invalid number %s\n", 
                    timeNow(), optarg);
                exit(1);
            }

            if (number == 's') startValue = value * scale;
            else if (number == 'e') endValue = value * scale;
            else if (number == 'c') {
                if (value == 0) {
                    fprintf(stderr, "%s Error: invalid number %s\n", 
                        timeNow(), optarg);
                    exit(1);
                }
                chunkSize = value * scale;
            }
        }
    }

    if (endValue <= startValue) {
       fprintf(stderr, "%s Error: end value (%lld) is not larger than start value (%lld).\n", 
           timeNow(), endValue, startValue);
       exit(1);
    }

    if (strchr(filePrefix, '/') || strchr(fileInfix, '/') || strchr(fileSuffix, '/')) {
       fprintf(stderr, "%s Error: '/' found in file name, directories MUST be specified using --dir\n", 
           timeNow(), endValue, startValue);
       exit(1);
    }

    fileCount = optind - argC;
    files = argV + optind;
}



int main(int argC, char ** argV) {
    parseArgs(argC, argV);

    if (fileCount == 0) initializeSelf();
    else initializeFromFile();

    processAll();
    
    if (fileCount == 0) finalSelf();
    else finalFile();
    return 0;
}

