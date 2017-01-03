# prime(1) - Generate prime numbers

## SYNOPSIS
`prime [options ...]`  
`prime-slow [options ...]`  
`prime-64 [options ...]`  
`prime-gmp [options ...]`  

## DESCRIPTION
Generates prime numbers very rapidly.

`prime` - The default prime number generator (ie: `prime-64`)

`prime-slow` - A very simplistic generator designed to be reliable yet very slow.  This is very good for verifying a tool chain.  Generating primes above 1 billion is not advisable.  The code is simple to avoid bugs and so the results are trustworty. Note that most of the options listed here do not affect `prime-slow`

`prime-64` - A very fast generator which scans large blocks (default 1 billion - 1,000,000,000).  This is extremely fast and can easily fill hard drives with prime numbers even at the end of its 64 bit signed integer range.

`prime-gmp` - A little slower than prime-64 but is based on the same technique.  Operations are carried out using low level `libgmp` functions instead of native 64 bit integers.  As a result it can work in 128 bits.  The main limitation to the maximum prime is based on the size of initialization.  When using self initialisation, this is generated in a single threaded fassion and in process memory.  As an alternative you can generate a very large initialisation file which can then be memory mapped. See `-i` flag.

## OPTIONS
*  `-a` `--text-out`:
Sets the output file mode to ASKII text.  Each prime number will be written on its own line.  This is default so is only really there to override the `-b` or `-B` flag.  Which ever of them is last will take precident.

*  `-b` `--binary-out`:
Sets the output file mode to binary.  This can be used to generate initialisation files for later use with the `-i` flag.  Each prime will be written in the native format.  For `prime-64` this will be a 64 bit signed integer - the endianess will depend of the system architechture.  For `prime-gmp` the format will be an array of system dependent integers ordered low to high segnificance (little endian).  Byte order low to high will be...  
    Little endian 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16.  
    Big endian 32bit 4 3 2 1 8 7 6 5 12 11 10 9 16 15 14 13.  
    Big endian 64bit 8 7 6 5 4 3 2 1 16 15 14 13 12 11 10 9.

* `-B` `--compressed-out`:
Sets the output to a higly compressed format.  Each byte represents 8 odd numbers starting with the lowest in the requested range.  The low segnificant bit represents the smallest up to the high segnificance bit representing the largest.  Each bit will be 1 if the number is prime or 0 if it is not prime.  This is by far the most compressed format and is also the fastest to generate.

* `-c` <size>  `--chunk-size` <size>:
Sets the chunk size to be processed.  This should be larg enough to improve performance but not so large that requires too much RAM.  Each thread will require the chunksize / 16 bytes of RAM to function. Suffix this with K,M,G,T to multiply by one thousand, million, billion or trillion respectivly.  The default chunk size is 1G (1,000,000,000) and requires 62,500,000 bytes of RAM per thread.  Note that the chunk size will also be used to break up the files if the

* `-d` <directory>  `--dir` <directory>:
Specifies the directory to place the output files.  Note that specifying an absolute path in the file name (one starting with /) will override this.  By default output files are placed in the current working directory. 

* `-e` <num> `--end` <num>:  
Sets the end of the search range.  Suffix this with K,M,G,T to multiply by one thousand, million, billion or trillion respectivly.  The choice of suffix will affect the default naming convention of the output file.
 
* `-f` `--multi-file`:
Sets prime to write to files.  Each chunk will be written to its own file.  This is recommended when multithreading to reduce the contention between threads.

* `-F` `--single-file`:
Sets prime to write to a single file instead multiple or stdout. All chunks will be written in order which will slow the program down.  It is better to use `--multi-file` for speed.

* `-h` `--help`:
Prints out command help

* `-i` <init file> `init-file` <init file>:
Use init file

* `-I` `--create-init-file`:
Equivalent to `--binary-out --single-file --start 3 --name init-%9e9OG.dat`.

* `-k` `--clobber`:
Allow overwriting of existing files.  By default prime will cause the job to abort.

* `-l` <num> `--low-prime-max` <num>: 
Set the size of the largest "low prime".  Default is 19.  Max is 23 due to memory constraints.  Low primes require a bitmap to be created and shared between the threads.  The size of the bitmap is the product of every low prime (excluding 2), so the default "19" causes a bitmap of 3*5*7*11*13*17*19 = 4,849,845 = 4.63MiB.  The max of 23 requires 2.18GiB.

* `-n` <name> `--file-name` <name>:
Sets the name pattern for the output file name.  See FILE NAME FORMATS

* `-p` `--use-stdout`:
Writes all output to the stdout instead of files.  Like with `-f` each chunk will be written in order forcing threads to wate for each other.

* `-P` `--post-process` <command>:
Pushes all content through the specified command.  To pass through arugments then wrap the args with quotes along with the command. Eg: `prime --post-process 'gzip -9'`

* `-q` `--quiet`:
Switchs off all writing to the stderr except for errors.  Normally prime will write its progress, this flag turns that off.

* `-s` <num> `--start` <num>:
Sets the start of the search range. Suffix this with K,M,G,T to multiply by one thousand, million, billion or trillion respectivly.  The choice of suffix will affect the default naming convention of the output file.

* `-S` `--stats-out`:
Scans for primes but instead of writing all primes out, just the stats for the chunk are written.

* `-v` `--verbose`:
Switches on full verbose logging to stderr.  This will be overridden by `-q`.

* `-x` <n>  `--threads` <n>:
Specifies the number of threads to use (default 1).

## FILE NAME FORMATS
Special characters

* `%%`:
A single %

* `%`<k>`e`<j>`o`:
The start value divided by 10^j padded to at least k digits.  Eg: `%12e6om` is the start value in millions padded to 12 digits.

* `%`<k>`e`<j>`O`:
The end value divided by 10^j padded to atleast k digits.  Eg: `%12e6Om` is the end value in millions padded to 12 digits.

* `%x`:
The thread number

* `%t`:
The current time

* `%d`:
The current date


The default file name depends on which suffixes have been used in `-s` `-e` and `-c` flags.  Based on the lowest of these three, one of the following defaults will be used for the file name:

* `prime.%9e9oG-%9e9OG.txt`:
G  or larger suffix

* `prime.%12e6oM-%12e6OM.txt`:
M  suffix

* `prime.%15e3oK-%15e3OK.txt`:
K  suffix

* `prime.%18e0o-%18e0O.txt`:
no suffix

## KNOWN ISSUES
Initialisation files have been removed as it was found that IO rates caused them to run very slowly.  These are planned to be re-introduced in a later version but using a different system.  The option to generate an initialisation file has been left in, but the files it generates will not be compatible with any future initialisation feature.

## EXAMPLES

Simplest use case.  Generate every primme up to 1 billion in a file in the current working directory (single threaded).

Run single threaded writing every prime number up to 100 billion (100,000,000,000)  
`prime --end 100G --use-stdout > result.txt`

Run multi threaded to generate every prime up to 1 trillion (1,000,000,000,000) writing compressed format.
`prime --end 1T --compressed-out`

## AUTHOR(S)
Philip Couling