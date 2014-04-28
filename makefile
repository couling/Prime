flags= -std=c99 -O3 -s
c_files= $(filter %.c, $^)
output_name= -o $@
basic_depends= prime_shared.h makefile | build
arch=`cat arch`
version=`cat version`
package=`awk '/Package:/ { print $2 }' control`_${version}_${arch}.deb

all: build/prime-64 build/prime-gmp build/prime-slow build/prime.1.gz

package: ${pacakge}

build/prime-64: prime.c prime_64.c prime_shared.c prime_64.h $(basic_depends)
	gcc $(output_name) $(flags) -DPRIME_ARCH_INT $(c_files)  -lm -lpthread
	
build/prime-gmp: prime.c primegmp.c prime_shared.c primegmp.h $(basic_depends)
	gcc $(output_name) $(flags) -DPRIME_ARCH_GMP $(c_files) -lgmp -lpthread

build/prime-slow: prime-slow.c prime_64.c prime_shared.c prime_64.h $(basic_depends)
	gcc $(output_name) $(flags) -DPRIME_ARCH_INT $(c_files) -lm -lpthread

build/prime.1.gz: prime.1
	gzip -9c prime.1 > build/prime.1.gz 

build/control: control manifest arch version
	cat control > build/control
	echo Architecture: ${arch} >> build/control
	echo Version: ${version} >> build/control

${package}: all build/control
	package-project . manifest build

clean:
	rm -rf build


dirs: build lib

.PHONY :  dirs clean all

build lib:
	mkdir $@
	
cls:
	cls


