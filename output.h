#ifndef output_h
#define output_h

#include "prime_shared.h"

#define COMPRESSED_BINARY_SIGNATURE "Compressed Prime Binary: 1.0"

// Header for compressed binary
typedef struct {            // All header fields are text (UTF-8) NOT binary
    char signature[32];     // Literally: "Compressed Prime Binary: 1.0"
    char headerSize[32];    // The size of this structure: sizeof(CompressedBinaryHeader)
                            // Extensions to this format may increase this from 1024 adding additional fields at the endRange
                            // Extensions should keep this size as a multiple of 1024.
    char dataBlockSize[32]; // the size of the data block following this header (protection against truncated files and allowance for concatenated files).
    char fromToSize[32];    // The size of the from and to fields in this header (ie: 256)
    char primeCount[32];    // The number of primes found in this file
    char textSize[32];      // The number of bytes in an askii representation of this file this includes one byte per prime for a delimiting \n character.
    char comments[288];     // Anything may be written here as long as it's UTF-8
    char skip[32];          // Comma separated list of primes who's multiples are not in the bitmap.  This is current just "2".
    char from[256];         // The offset for this bitmap including skipped numbers.
                            // Eg: from "1000" skip "2" - the first bit will represent 1001.
                            // Eg: from "5000" skip "2,3" - the first bit will represent 5003.
    char to[256];           // The upper limit of this data file.  De-compressors MUST ignore all bits >= to this.
} __attribute__ ((packed)) CompressedBinaryHeader;



#define getPrimeFromMap(value, offset, byteIndex, bitIndex) {\
    prime_set_num(value, byteIndex);\
    prime_mul_16(value, value);\
    prime_add_num(value, value, bitIndex);\
    prime_add_prime(value, value, offset);\
}
#endif
