all: build/prime-64 build/prime-gmp build/prime-slow

build/prime-64: prime.c prime_64.c prime_shared.c | build
	gcc -std=c99 -DPRIME_64 -O3 -s -o $@ $^ -lm -lpthread
	
build/prime-gmp: prime.c prime_64.c prime_shared.c | build
	gcc -std=c99 -DPRIME_GMP -O3 -s -o $@ $^ -lm -lpthread -lgmp


build/prime-slow: prime-slow.o prime_64.o prime_shared.o | build
	gcc -O3 -s -o $@ $^ -lm



clean:
	rm -rf build
	rm -rf obj


dirs: build lib

.PHONY :  dirs clean all

build lib:
	mkdir $@
	
cls:
	cls


