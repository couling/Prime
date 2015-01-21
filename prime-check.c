#include <stdio.h>
#include <errno.h>

#include "prime_shared.c"

typedef char Buffer[1024];

static char * file1;
static char * file2;

static void configure(int argC, char ** argV) {
    comparisonFileName = argV[1];
    fileCount = argC+2;
    fileNames = argV-1;
}

static int compareString(Buffer * b1, Buffer * b2) {
    size_t len1 = strlen(b1);
    size_t len2 = strlen(b2);
    if (len1 < len2) return -1;
    if (len1 > len2) return 1;
    return strcmp(b1,b2);
}

static int compareFiles(FILE * f1, FILE * f2) {
    Buffer buffer1;
    Buffer buffer2;

    if (fgets(buffer1, sizeof(Buffer), file1) && fgets(buffer2, sizeof(Buffer), file2)) {
        int comparison = compareString(buffer1, buffer2);
        while (true) {
            if (comparison > 0) {
                int count=1;
                Buffer from;
                strcpy(from, buffer2);
                do {
                    if (!fgets(buffer2, sizeof(Buffer), file2)) break;
                    comparison = compareString(buffer1, buffer2);
                    ++count;
                } while (comparison > 0);
                if (count == 1) printf("gained: %s", from);
                else printf("gained: %s to %s (%d)", from, buffer2, count);
                if (comparison > 0) break;
            }
            else if (comparison < 0) {
                int count=1;
                Buffer from;
                strcpy(from, buffer1);
                do {
                    if (!fgets(buffer1, sizeof(Buffer), file1)) break;
                    comparison = compareString(buffer1, buffer1);
                    ++count;
                } while (comparison < 0);
                if (count == 1) printf("missed: %s", from);
                else printf("missed: %s to %s (%d)", from, buffer2, count);
                if (comparison < 0) break;
            }
            else {
                if (!fgets(buffer1, sizeof(Buffer), file1)) break;
                if (!fgets(buffer2, sizeof(Buffer), file2)) break;
                comparison = compareString(buffer1, buffer2);
            }
        }
    }
    
    if (feof(f1)) {
        int count = 0;
        Buffer from;
        strcpy(from, buffer2);
        while (fgets(buffer2, sizeof(Buffer), file2)) ++count;
        if (count == 1) printf("gained: %s", from);
        else printf("gained: %s to %s (%d)", from, buffer2, count);
    }
    else if (feof(f2)) {
        int count = 0;
        Buffer from;
        strcpy(from, buffer1);
        while (fgets(buffer1, sizeof(Buffer), file1)) ++count;
        if (count == 1) printf("gained: %s", from);
        else printf("gained: %s to %s (%d)", from, buffer1, count);
    }
}

int main(int argC, char ** argV) {
    FILE * comparisonFile = fopen(comparisonFileName ,"r");
    if (comparisonFile == NULL) exitError(1,errno,"Could not open file: %s", comparisonFileName);
    FILE ** files = mallocSafe(sizeof(FILE *) * fileCount);
    for (int i=0; i< fileCount; ++i) {
        files[i] = fopen(fileNames[i] ,"r");
        if (files[i] == NULL) exitError(-1,errno,"Could not open file: %s", files[i]);
    }
    int comparison = compareFiles(comparisonFile, files);
    fclose(comparisonFile);
    for (int i=0; i<fileCount) {
        fclose(files[i]);
    }
    free(files);
    return comparison;
}
