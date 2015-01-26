#include "shared.h"

#define _GNU_SOURCE 
#include <time.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <stdarg.h>

#include <fcntl.h> 
#include <unistd.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <wordexp.h>

// For thread local storage
pthread_key_t threadNumKey;

#define PIPE_READ 0
#define PIPE_WRITE 1


void initializeThreading() {
    pthread_key_create(&threadNumKey, NULL);
}



char * timeNow() {
    time_t rawtime;
    struct tm * timeinfo;
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    static char t[100];
    strftime(t, 100, "%a %b %d %H:%M:%S %Y", timeinfo);
    return t;
}



void stdLog(const char * str, ...) {
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



void exitError(int num, int errorNumber, const char * str, ...) {
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



void logWarning(int errorNumber, const char * str, ...) {
    int * threadNumPt = pthread_getspecific(threadNumKey);
    int threadNum = threadNumPt ? *threadNumPt : 0;
    flockfile(stderr);
    fprintf(stderr, "%s [%02d] Warning: ", timeNow(), threadNum);
    va_list ap;
    va_start(ap, str);
    vfprintf(stderr, str, ap);
    va_end(ap);
    fprintf(stderr, "\n");
    if (errorNumber)
        fprintf(stderr, "%s [%02d] Warning: %s\n", timeNow(), threadNum, strerror(errorNumber));
    funlockfile(stderr);
}



void * mallocSafe(size_t bytes) {
    void * result = malloc(bytes);
    if (!result) exitError(255, errno, "Could not allocate to %zd bytes", bytes);
    return result;
}



void * reallocSafe(void * existing, size_t bytes) {
    void * result = realloc(existing, bytes);
    if (!result) exitError(255, errno, "Could not reallocate to %zd bytes", bytes);
    return result;
}



void mkdirs(char * formattedFileName) {
    // Create necessary directories
    for (char * cptr = formattedFileName; *cptr != '\0'; ++cptr) {
        if (*cptr == '/') {
            *cptr = '\0';
            if (!mkdir(formattedFileName, S_IRWXU)) {
                stdLog("Created directory for output: %s", formattedFileName);
            }
            *cptr = '/';
        }
    }
}



void closeAndWait(ChildProcess * process) {
    if (process->stdin >= 0 && close(process->stdin))
        exitError(-1,errno, "Failed to close stdin for %s",  process->command);
    if (process->stdout >= 0 && close(process->stdout))
        exitError(-1,errno, "Failed to close stdout for %s", process->command);

    int status;
    if (waitpid(process->processID, &status, 0) == -1) {
        exitError(-1, errno, "Could not wait for %s", process->command);
    }

    if (!WIFEXITED(status)) exitError(-1, 0, "Command did not exit properly %s", process->command);
    if (WEXITSTATUS(status)) exitError(-1, 0, "Command %s returned %d not 0", process->command, WEXITSTATUS(status));
    process->processID = 0;
}



void execPipeProcess(ChildProcess * process, const char* szCommand, int in, int out) {
    // Expand any args
    wordexp_t words;
    if (wordexp (szCommand, &words, 0)) exitError(-1, 0, "Could not expand command %s\n", szCommand);


    // Runs the command
    char nChar;
    int nResult;

    if (in < 0) {
        int aStdinPipe[2];
        if (pipe(aStdinPipe) < 0) {
            exitError(-1, errno, "allocating pipe for child input redirect failed");
        }
        process->stdin = aStdinPipe[PIPE_WRITE];
        in = aStdinPipe[PIPE_READ];
    }
    else {
        process->stdin = -1;
    }
    if (out < 0) {
        int aStdoutPipe[2];
        if (pipe(aStdoutPipe) < 0) {
            exitError(-1, errno, "allocating pipe for child input redirect failed");
        }
        process->stdout = aStdoutPipe[PIPE_READ];
        out = aStdoutPipe[PIPE_WRITE];
    }
    else {
        process->stdout = -1;
    }

    process->processID = fork();
    if (0 == process->processID) {
        // child continues here

        // these are for use by parent only
        if (process->stdin >= 0) close(process->stdin);
        if (process->stdout >= 0) close(process->stdout);

        // redirect stdin
		if (STDIN_FILENO != in) {
            if (dup2(in, STDIN_FILENO) == -1) {
              exitError(-1, errno, "redirecting stdin failed");
            }
		}

        // redirect stdout
		if (STDOUT_FILENO != out) {
            if (dup2(out, STDOUT_FILENO) == -1) {
              exitError(-1, errno, "redirecting stdout failed");
            }
		}


        // redirect stderr
        /*
        maybe in another program
        if (dup2(aStdoutPipe[PIPE_WRITE], STDERR_FILENO) == -1) {
            exitError(-1, errno, "redirecting stderr failed");
        }*/

        // we're done with these; they've been duplicated to STDIN and STDOUT
        close(in);
        close(out);

        // run child process image
        // replace this with any exec* function find easier to use ("man exec")
        // environ is declared in unistd.h
        nResult = execvp(words.we_wordv[0], words.we_wordv);

        // if we get here at all, an error occurred, but we are in the child
        // process, so just exit
        exitError(-1, errno, "could not run %s", szCommand);
  } else if (process->processID > 0) {
        wordfree(&words);
        // parent continues here

        // close unused file descriptors, these are for child only
        close(in);
        close(out);
        process->command = szCommand;
    } else {
        exitError(-1,errno, "Failed to fork");
    }
}

