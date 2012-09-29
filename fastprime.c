# This program has not been finished yet.

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define PRIME_TYPE long long
#define PRINT_PRIME(p) printf("%lld\n", p)

typedef PRIME_TYPE Value;

typedef struct {
	Value value;
	Value nextUsed;
} Prime;

Value startValue;
Value endValue;
Value reportPeriod;
int queueSize;
Prime * primeHeap;
Prime ** primeQueue;

char timeNow[100];

void setTime() {
	time_t rawtime;
	struct tm * timeinfo;
	time ( &rawtime );
	timeinfo = localtime ( &rawtime );
	
	strftime (timeNow,100,"%a %b %d %H:%M:%S %Y",timeinfo);
}

void cyclePrimeInQueue() {
	primeQueue[0]->nextUsed += (primePipeline[nextPrimeInPipe].value << 1);
	int currentPos = 0;
	int done = false;
	do {
		int branch1 = (currentPos+1) * 2;
		int branch2 = branch1-1;
		// We will always have a balanced tree
		// Therefore if branch 1 exists, so will branch 2
		if (branch1 >=  queueSize) {
			done = 1;
		}
		else if (primeQueue[currentPos]->nextUsed > primeQueue[branch1]->nextUsed) {
			if (primeQueue[branch1]->nextUsed < primeQueue[branch2]->nextUsed) {
				Prime * tmp = primeQueue[branch1];
				primeQueue[branch1] = primeQueue[currentPos];
				primeQueue[currentPos] = tmp; 
				currentPos = branch1;
			}
			else {
				Prime * tmp = primeQueue[branch2];
				primeQueue[branch2] = primeQueue[currentPos];
				primeQueue[currentPos] = tmp; 
				currentPos = branch2;
			}
		}
		else if (primeQueue[currentPos]->nextUsed > primeQueue[branch2]->nextUsed) {
			Prime * tmp = primeQueue[branch2];
			primeQueue[branch2] = primeQueue[currentPos];
			primeQueue[currentPos] = tmp;
			currentPos = branch2;
		}
		else {
			done = 1;
		}
	} while (!done);
}

Value init(int argC, char ** argV) {
	setTime();
	fprintf(stderr, "%s Initializing", timeNow);
	primeHeap = calloc(TOTAL_PRIMES,sizeof(Prime));
	primeQueue = calloc(TOTAL_PRIMES,sizeof(Prime *));
	if (!primeQueue || !primePipeLine) {
		setTime();
		fprintf(stderr, "%s Could not allocate %d bytes for prime storage.\n", timeNow, TOTAL_PRIMES);
		exit(1);
	}
	Value two = 2;
	primeHeap[0].value = 3;
	primeHeap[0].nextUsed = 9;
	primeHeap[1].value = 5;
	primePipeLine[1].nextUsed = 25;
	primeQueue[0] = &(primePipeLine[0])
	primesInPipe = 2;
	nextPrimeInPipe = 1;
	primesInQueue = 1;
	
	// These primes are hard coded for completeness we print them out.
	
	PRINT_VALUE(two);
	PRINT_VALUE(primePipeline[0].value);
	PRINT_VALUE(primePipeLine[1].value);
	return 5; // this version of the code always starts from 5
}

void finish() {
	setTime();
	fprintf(stderr, "%s Finshing\n", timeNow);
	free(primeQueue);
	free(primePipeline);
	setTime();
	fprintf(stderr, "%s Finished\n", timeNow);
}

Value process() {
	Value value = startValue;
	setTime();
	fprintf(stderr, "%s Processing values from %lld\n", timeNow);
	 // we have to start somewhere so we might as well start on a prime
	while (value < endValue) {
		Value nextReport = value + reportPeriod;
		if (nextReport > endValue) nextReport = endValue;
	 
		while (value < nextReport) {
			if (primeQueue[0]->nextUsed == value) {
				foundFactor = 1;
				do cyclePrimeInQueue();
				while (primeQueue[0]->nextUsed == value);
			}
			else {
				PRINT_VALUE(value);
			}
			value +=2
		}
		
		setTime();
		fprintf(stderr, "%s Processing Low Values\n", timeNow);
	}
	
	return value;
}

int main(int argC, char ** argV) {
	Value value;
	value = init(argC, argV);
	value = processLow(value);
	value = processHigh(value);
	finish();
	return 0;
}

