#include <stdio.h>
#include <errno.h>
#include <string.h>
#include "shared.h"

typedef char Buffer[1024];

static char * fileName1;
static char * fileName2;

static void configure(int argC, char ** argV) {
    fileName1 = argV[1];
	fileName2 = argV[2];
}

static int compareString(Buffer b1, Buffer b2) {
    size_t len1 = strlen(b1);
    size_t len2 = strlen(b2);
    if (len1 < len2) return -1;
    if (len1 > len2) return 1;
    return strcmp(b1,b2);
}

static void printDiff(const char * diff, Buffer from, Buffer to, int count) {
	int s = strlen(from);
	if (from[s-1] == '\n') from[s-1] = '\0';
	s = strlen(to);
	if (to[s-1] == '\n') to[s-1] = '\0';
	if (count == 1) printf("%s: (%d) %s\n", diff, count, from);
	else printf("%s: (%d) %s to %s\n", diff, count, from, to);
}

static int compareFiles(FILE * file1, FILE * file2) {
	int diffFound = 0;
    Buffer buffer1;
    Buffer buffer2;

    if (fgets(buffer1, sizeof(Buffer), file1) && fgets(buffer2, sizeof(Buffer), file2)) {
        int comparison = compareString(buffer1, buffer2);
        while (1) {
            if (comparison > 0) {
				diffFound = 1;
                int count=0;
                Buffer from;
				Buffer to;
                strcpy(from, buffer2);
                do {
                    if (!fgets(buffer2, sizeof(Buffer), file2)) break;
                    comparison = compareString(buffer1, buffer2);
                    ++count;
					strcpy(to, buffer2);
                } while (comparison > 0);
				printDiff("gained", from, to, count);
                if (comparison > 0) break;
            }
            else if (comparison < 0) {
				diffFound = 1;
                int count=0;
                Buffer from;
				Buffer to;
                strcpy(from, buffer1);
                do {
                    if (!fgets(buffer1, sizeof(Buffer), file1)) break;
                    comparison = compareString(buffer1, buffer2);
                    ++count;
					strcpy(to, buffer1);
                } while (comparison < 0);
                printDiff("missed", from, to, count);
                if (comparison < 0) break;
            }
            else {
                if (!fgets(buffer1, sizeof(Buffer), file1)) break;
                if (!fgets(buffer2, sizeof(Buffer), file2)) break;
                comparison = compareString(buffer1, buffer2);
            }
        }
    }
    
    if (feof(file1)) {
        int count = 0;
        Buffer from;
		Buffer to;
        strcpy(from, buffer2);
        while (fgets(buffer2, sizeof(Buffer), file2)) {
			++count;
			strcpy(to, buffer2);
		}
		if (count != 0) {
			diffFound = 1;
			printDiff("gained", from, to, count);
		}
    }
    else if (feof(file2)) {
        int count = 1;
        Buffer from;
        Buffer to;
		strcpy(from, buffer1);
        while (fgets(buffer1, sizeof(Buffer), file1)) {
			++count;
			strcpy(to, buffer1);
		}
		if (count != 0) {
			diffFound = 1;
            printDiff("missed", from, to, count);
		}
    }
	return diffFound;
}

int main(int argC, char ** argV) {
	configure(argC, argV);
    FILE * file1 = fopen(fileName1 ,"r");
    if (file1 == NULL) exitError(1,errno,"Could not open file 1: %s", fileName1);
	FILE * file2 = fopen(fileName2 ,"r");
	if (file2 == NULL) exitError(1,errno,"Could not open file 2: %s", fileName2);
    int comparison = compareFiles(file1, file2);
    fclose(file1);
	fclose(file2);
    return comparison;
}
