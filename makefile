flags= -std=c99 -O3 -s
c_files= $(filter %.c, $^)
output_name= -o $@
basic_depends= prime_shared.h makefile | build

gcc_arch:=${shell gcc -dumpmachine | awk -F- '{print $$1}' }
arch:=${or ${if ${filter ${gcc_arch},x86_64},amd64}, ${filter x86,${gcc_arch}}}
version:=${shell awk '/^Version:/ {print $$2}' control}r${shell svn info | awk '/Revision:/ {print $$2}'}
package:=${shell awk '/^Package:/ { print $$2 }' control}_${version}_${arch}.deb
required_files:=${filter-out build/control,${shell awk -F':' '/^[ \t]*file:/ { print $$2 }' manifest}}

all: ${required_files}

package: build/${package}

build/prime-64: prime.c prime_64.c prime_shared.c prime_64.h $(basic_depends)
	gcc $(output_name) $(flags) -DPRIME_ARCH_INT -DPRIME_PROGRAM_NAME=prime-64 -DPRIME_PROGRAM_VERSION="${version} ${arch}"  $(c_files)  -lm -lpthread

build/prime-gmp: prime.c prime_gmp.c prime_shared.c prime_gmp.h $(basic_depends)
	gcc $(output_name) $(flags) -DPRIME_ARCH_GMP -DPRIME_PROGRAM_NAME=prime-gmp -DPRIME_PROGRAM_VERSION="${version} ${arch}" -DPRIME_SIZE=128 $(c_files) -lgmp -lpthread

build/prime-slow: prime-slow.c prime_64.c prime_shared.c prime_64.h $(basic_depends)
	gcc $(output_name) $(flags) -DPRIME_ARCH_INT -DPRIME_PROGRAM_NAME=prime-slow -DPRIME_PROGRAM_VERSION="${version} ${arch}" $(c_files) -lm -lpthread

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


