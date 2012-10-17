#!/bin/sh
 
gcc -Werror -std=c99 -O3 -s -o prime bitprime.c

exit $?
