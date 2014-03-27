all: build/prime-64 build/prime-gmp build/prime-slow

build/prime-64: prime.c prime_64.c prime_shared.c prime_64.h prime_shared.h makefile | build
	gcc -std=c99 -DPRIME_ARCH_INT -O3 -s -o $@ prime.c prime_64.c prime_shared.c -lm -lpthread
	
build/prime-gmp: prime.c primegmp.c prime_shared.c primegmp.h prime_shared.h makefile  | build
	gcc -std=c99 -DPRIME_ARCH_GMP -O3 -s -o $@ prime.c primegmp.c prime_shared.c -lgmp -lpthread

build/prime-slow: prime-slow.o prime_64.o prime_shared.o | build
	gcc -O3 -s -o $@ $^ -lm



clean:
	rm -rf build


dirs: build lib

.PHONY :  dirs clean all

build lib:
	mkdir $@
	
cls:
	cls


