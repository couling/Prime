flags= -std=c99 -O3 -s
c_files= $(filter %.c, $^)
depends_basic= shared.c shared.h makefile | build
depends_prime_64=  prime_64.c prime_64.h prime_shared.c prime_shared.h $(depends_basic)
depends_prime_128=  prime_gmp.c prime_gmp.h prime_shared.c prime_shared.h $(depends_basic)

gcc_arch:=${shell gcc -dumpmachine | awk -F- '{print $$1}' }
arch:=${or ${if ${filter ${gcc_arch},x86_64},amd64}, ${filter ${gcc_arch},x86}, ${if ${filter ${gcc_arch},arm},armhf}}
version:=${shell awk '/^Version:/ {print $$2}' control}r${shell svn info | awk '/Revision:/ {print $$2}'}
package:=${shell awk '/^Package:/ { print $$2 }' control}_${version}_${arch}.deb
required_files:=${filter-out build/control,${shell awk -F':' '/^[ \t]*file:/ { print $$2 }' manifest}}

version_injection=-DPRIME_PROGRAM_NAME=$(subst build/,,$@) -DPRIME_PROGRAM_VERSION="${version} ${arch}"
arch_64_injection=-DPRIME_ARCH_INT $(version_injection)
arch_128_injection=-DPRIME_ARCH_GMP -DPRIME_SIZE=128 $(version_injection)

lib_basic= -lpthread
lib_64= -lm $(lib_basic)
lib_128= -lgmp $(lib_basic)

all: ${required_files}

package: build/${package}

build/prime-64: prime.c $(depends_prime_64)
	gcc -o $@ $(flags) $(arch_64_injection) $(c_files) $(lib_64)

build/prime-64-nolog: prime.c $(depends_prime_64)
	gcc -o $@ $(flags) $(arch_64_injection) -DSTRIP_LOGGING $(c_files) $(lib_64)

build/prime-gmp: prime.c $(depends_prime_128)
	gcc -o $@ $(flags) $(arch_128_injection) $(c_files) $(lib_128)

build/prime-slow: prime-slow.c $(depends_prime_64)
	gcc -o $@ $(flags) $(arch_64_injection) $(c_files) $(lib_64)

build/prime-check: prime-check.c $(depends_basic)
	gcc -o $@ $(flags) $(c_files) $(lib_basic)

build/prime.1.gz: prime.1
	gzip -9c prime.1 > build/prime.1.gz 

build/control: control | build
	cp control build/control
	echo Architecture: ${arch} >> build/control

build/${package}: ${required_files} manifest build/control
	package-project manifest
	mv ${package} build/${package}

push: build/${package}
	package-push build/${package}

clean:
	rm -rf build


build:
	mkdir $@


.PHONY:  dirs clean all package push


