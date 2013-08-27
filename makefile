all: build/prime build/prime-slow


clean:
	rm -rf build || true
	rm -rf obj || true
	rm -rf lib || true


all: build/prime build/prime-slow

dirs: build obj lib

.PHONY :  dirs clean all

build obj lib:
	mkdir $@
	
cls:
	cls


obj/%.o: %.c makefile | obj
	gcc -o $@ -O3 -c -Werror -std=c99 -MMD $<

-include obj/*.d


build/prime: obj/prime.o obj/prime_64.o obj/prime_shared.o | build
	gcc -O3 -s -o $@ $^

build/prime-slow: obj/prime-slow.o | build
	gcc -O3 -s -o $@ $^
