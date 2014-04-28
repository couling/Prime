flags= -std=c99 -O3 -s
c_files= $(filter %.c, $^)
output_name= -o $@
basic_depends= prime_shared.h makefile | build
required_files:=`awk -F':' '/^[ \t]*file:/ { print $2 }' manifest`
arch:=`cat arch`
version:=`cat version`
package:=`awk '/^Package:/ { print $2 }' control`_${version}_${arch}.deb

all: ${required_files}

package: ${package}

build/prime-64: prime.c prime_64.c prime_shared.c prime_64.h $(basic_depends)
	gcc $(output_name) $(flags) -DPRIME_ARCH_INT $(c_files)  -lm -lpthread

build/prime-gmp: prime.c primegmp.c prime_shared.c primegmp.h $(basic_depends)
	gcc $(output_name) $(flags) -DPRIME_ARCH_GMP $(c_files) -lgmp -lpthread

build/prime-slow: prime-slow.c prime_64.c prime_shared.c prime_64.h $(basic_depends)
	gcc $(output_name) $(flags) -DPRIME_ARCH_INT $(c_files) -lm -lpthread

build/prime.1.gz: prime.1
	gzip -9c prime.1 > build/prime.1.gz 

build/control: control arch version
	cp control build/control
	echo Architecture: ${arch} >> build/control
	echo Version: ${version} >> build/control

${package}: ${required_files} manifest makefile
	package-project . manifest build

clean:
	rm -rf build


dirs: build lib

.PHONY :  dirs clean all package

build lib:
	mkdir $@

cls:
	cls


