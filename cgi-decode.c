#include "output.h"
#include "prime_shared.h"
#include "shared.h"

#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>


#define BUFFER_SIZE 0x100000
#define SCAN_DEBUG_MASK 0x3FFFFF

static unsigned char checkMask[] =  {0x00, 0x01, 0x00, 0x02, 0x00, 0x04, 0x00, 0x08, 0x00, 0x10, 0x00, 0x20, 0x00, 0x40, 0x00, 0x80};

static int  bitCount[] =            {0,1,1,2,1,2,2,3,1,2,2,3,2,3,3,4,1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,
                                     1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
                                     1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
                                     2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
                                     1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
                                     2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
                                     2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
                                     3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,4,5,5,6,5,6,6,7,5,6,6,7,6,7,7,8};


static void exitDiscoveryMismatch(const char * message, long long count) {
    char * word;
    if (count > 0) {
        word = "fewer";
    }
    else {
        word = "more";
        count = - count;
    }

    exitError(1,0,message, count, word);
}



static void writeSafe(int file, const void * buffer, size_t size) {
    if (write(file, buffer, size) != size) exitError(1, errno, "Failed to write prime file");
}



static size_t readSafe(int fd, void * buffer, size_t count, const char * fileName) {
    size_t bytesRead = read(fd, buffer, count);
    if (bytesRead == -1) exitError(1,errno,"Could not read from %s", fileName);
    return bytesRead;
}



// TODO Add file name to this
static size_t stringToSizeT(const char * string) {
    size_t value;
    if (sscanf(string, "%zd", &value) != 1) exitError(1,0,"Invalid number found in header: %s", string);
    return value;
}



// TODO add file name to this
static long long stringToLongLong(const char * string) {
    char * endptr;
    long long value = strtoll(string, &endptr, 10);
    if (*endptr || value < 0) {
        exitError(1, 0, "Invalid number found in header: %s", string);
    }
    return value;
}



static void writePrimeText(Prime from, Prime to, size_t range, int fd, int file, const char * fileName,
        long long expectedTextSize, long long primesExpected) {

    Prime prime_3;
    prime_set_num(prime_3, 3);
    int disallow2;
    disallow2 = prime_eq(from, prime_3);

    char writeBuffer[BUFFER_SIZE];
    unsigned char bitmap[BUFFER_SIZE];
    size_t readBufferContent=0;

    size_t remainingBuffer = BUFFER_SIZE;
    char * bufferWritePos = writeBuffer;

    Prime prime_2;
    prime_set_num(prime_2, 2);

    if (prime_le(from, prime_2) && prime_ge(to, prime_2) && !disallow2) {
        bufferWritePos[0] = '2';
        bufferWritePos[1] = '\n';
        bufferWritePos += 2;
        remainingBuffer -= 2;
        --primesExpected;
        expectedTextSize -=2;
    }

    size_t endRange = range -1;
    size_t readBufferPos = 0;
    for (size_t i = 0; i < endRange; ++i, ++readBufferPos) {
        if (readBufferPos >= readBufferContent) {
            if (range - i >= BUFFER_SIZE) readBufferContent = readSafe(fd, bitmap, BUFFER_SIZE, fileName);
            else readBufferContent = readSafe(fd, bitmap, range - i, fileName);
            if (!readBufferContent) exitError(1, 0,
                    "Unexpected end of file while decompressing %s, read %zd, expected %zd more bytes",
                    fileName, i, range - i);
            else readBufferPos = 0;
        }

        if (verbose && !(i & SCAN_DEBUG_MASK))
            stdLog("Writing primes as text %02.2f%%", 100 * ((double) i)/((double) range));

        if (bitmap[readBufferPos]) {
            if (remainingBuffer < PRIME_STRING_SIZE * bitCount[bitmap[readBufferPos]]) {
                writeSafe(file, writeBuffer, BUFFER_SIZE - remainingBuffer);
                bufferWritePos = writeBuffer;
                remainingBuffer = BUFFER_SIZE;
            }
            for (int j = 1; j < 16; j+=2) {
                if (bitmap[readBufferPos] & checkMask[j]) {
                    --primesExpected;
                    Prime value;
                    getPrimeFromMap(value, from, i, j);
                    int bytesWritten = prime_to_str(bufferWritePos, value);
                    expectedTextSize -= bytesWritten + 1;
                    bufferWritePos[bytesWritten] = '\n';
                    bufferWritePos += bytesWritten + 1;
                    remainingBuffer -= bytesWritten + 1;
                }
            }
        }
    }

    if (!readBufferPos >= readBufferContent) {
        readBufferContent = readSafe(fd, bitmap, 1, fileName);
        if (!readBufferContent) exitError(1, 0, "Unexpected end of file while decompressing %s", fileName);
        else readBufferPos = 0;
    }

    if (bitmap[readBufferPos]) {
       if (remainingBuffer < PRIME_STRING_SIZE * bitCount[bitmap[readBufferPos]]) {
            writeSafe(file, writeBuffer, BUFFER_SIZE - remainingBuffer);
            bufferWritePos = writeBuffer;
            remainingBuffer = BUFFER_SIZE;
        }
        for (int j = 1; j < 16; j+=2) {
            if (bitmap[readBufferPos] & checkMask[j]) {
                Prime value;
                getPrimeFromMap(value, from, endRange, j);
                if (prime_lt(value, to)) {
                    --primesExpected;
                    int bytesWritten = prime_to_str(bufferWritePos, value);
                    expectedTextSize -= bytesWritten +1;
                    bufferWritePos[bytesWritten] = '\n';
                    bufferWritePos += bytesWritten + 1;
                    remainingBuffer -= bytesWritten + 1;
                }
            }
        }
    }

    writeSafe(file, writeBuffer, BUFFER_SIZE - remainingBuffer);

    if (primesExpected)   exitDiscoveryMismatch("Decompressing found %lld %s primes than expected",   primesExpected);
    if (expectedTextSize) exitDiscoveryMismatch("Decompressing produced %lld %s bytes than expected", expectedTextSize);
}



#define forceNullTerminate(field) field[sizeof(field)-1] = '\0'

int readHeader(int file, CompressedBinaryHeader * header, const char * fileName) {
    size_t bytesRead = readSafe(file, header, sizeof(CompressedBinaryHeader), fileName);
    if (!bytesRead) return 0;

    while (bytesRead != sizeof(CompressedBinaryHeader)) {
        size_t newBytesRead = readSafe(file, ((void *)header) + bytesRead,
                                sizeof(CompressedBinaryHeader) - bytesRead, fileName);
        if (!bytesRead) exitError(1, 0, "Unexpected end of file while reading header.  Wanted %zd bytes, got %zd",
                sizeof(CompressedBinaryHeader), bytesRead);
        else bytesRead += newBytesRead;
    }


    // TODO accept different header sizes.
    // For now we'll just accept the only thing that my own code will generate and
    // double check that it matches

    forceNullTerminate(header->signature);
    forceNullTerminate(header->headerSize);
    forceNullTerminate(header->dataBlockSize);
    forceNullTerminate(header->fromToSize);
    forceNullTerminate(header->primeCount);
    forceNullTerminate(header->textSize);
    forceNullTerminate(header->comments);
    forceNullTerminate(header->skip);
    forceNullTerminate(header->from);
    forceNullTerminate(header->to);

    // Check that everything matches out expectations
    // Again this is built to check the file matches one that my
    // code generated, although the file supports more, this program currently doesn't.
    if (strcmp(COMPRESSED_BINARY_SIGNATURE, header->signature))
        exitError(1,0, "Invalid file header.  Wanted %s, found %s",
                COMPRESSED_BINARY_SIGNATURE, header->signature);

    if (stringToSizeT(header->headerSize) != sizeof(CompressedBinaryHeader))
        exitError(1,0, "File header declared invalid size. Wanted %zd, found %s",
                sizeof(CompressedBinaryHeader), header->headerSize);

    if (stringToSizeT(header->fromToSize) != sizeof(header->from))
        exitError(1,0, "File header declared invalid from/to size.  Wanted %zd, found %s",
                sizeof(header->fromToSize), header->fromToSize);

    if (strcmp("2", header->skip))
        exitError(1, 0, "File header declared invalid skip value.  Wanted 2. Found %s",
                header->skip);

    return 1;
}



static void printHeader(int inputFile, int output, const char * fileName) {
    CompressedBinaryHeader header;
    long long totalExpectedTextSize = 0;
    while (readHeader(inputFile, &header, fileName)) {
        long long expectedTextSize = stringToLongLong(header.textSize);
        long long bufferSize = stringToLongLong(header.dataBlockSize);
        totalExpectedTextSize += expectedTextSize;
        if (lseek(inputFile, bufferSize, SEEK_CUR) == -1) {
            exitError(1, errno, "Truncated file");
        }
    }
    
    lseek(inputFile, 0, SEEK_SET); 
    
    char buffer [200];
    size_t toPrint = snprintf(buffer, 200, "Content-Type:text/html;charset=utf-8\nContent-Length: %lld\n\n", totalExpectedTextSize);
    writeSafe(output, buffer, toPrint);
}


static void decompress(int inputFile, const char * fileName) {
    int outputFile = STDOUT_FILENO;
    printHeader(inputFile, outputFile, fileName);
    
    CompressedBinaryHeader header;
    while (readHeader(inputFile, &header, fileName)) {
        long long expectedTextSize = stringToLongLong(header.textSize);
        long long primesExpected = stringToLongLong(header.primeCount);
        long long bufferSize = stringToLongLong(header.dataBlockSize);
        Prime from, to;
        str_to_prime(from, header.from);
        str_to_prime(to, header.to);

        
        writePrimeText(from, to, bufferSize, inputFile, outputFile, fileName, expectedTextSize, primesExpected);
    }
}



static int openFile(const char * fileName) {
    int file = open(fileName, O_RDONLY);
    if (file == -1) exitError(1, errno, "Could not open input file %s", fileName);
    return file;
}



int main (int argC, char ** argV) {
    parseArgs(argC, argV);

	const char * file = getenv("PATH_TRANSLATED");
    decompress(openFile(file), file);

    return 0;
}
