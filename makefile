flags= -std=c99 -O3 -s
c_files= $(filter %.c, $^)
depends_basic= shared.c shared.h makefile | build
depends_prime_64= output.h prime_64.c prime_64.h prime_shared.c prime_shared.h $(depends_basic)
depends_prime_128= output.h prime_gmp.c prime_gmp.h prime_shared.c prime_shared.h $(depends_basic)

gcc_arch:=${shell gcc -dumpmachine | awk -F- '{print $$1}' }
arch:=${or ${if ${filter ${gcc_arch},x86_64},amd64}, ${filter ${gcc_arch},x86}, ${if ${filter ${gcc_arch},arm},armhf}}
version:=$(subst v,,${shell git describe --tags --match v[0-9].[0-9]*})
package:=${shell awk '/^Package:/ { print $$2 }' control}_${version}_${arch}.deb
required_files:=${filter-out build/control,${shell awk '$$1 == "file" { print $$2 }' manifest}}

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
	
build/prime-decompress-64: prime-decompress.c output.h $(depends_prime_64)
	gcc -o $@ $(flags) $(c_files) $(arch_64_injection) $(lib_basic) $(lib_64)

build/prime-cgi-decode-64: cgi-decode.c output.h $(depends_prime_64)
	gcc -o $@ $(flags) $(c_files) $(arch_64_injection) $(lib_basic) $(lib_64)

build/prime.1: prime.1.md makefile
	ronn -roff --manual='User Commands' --date='2013-10-01' --organization='Philip Couling' < prime.1.md > build/prime.1 

build/prime.1.gz: build/prime.1
	gzip -9 --force --keep build/prime.1

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


