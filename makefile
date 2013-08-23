all: prime prime-slow

prime: prime.c prime_64.h prime_64.c
	gcc -Werror -std=c99 -O3 -s -o prime prime.c prime_64.c

prime-slow: prime-slow.c
	gcc -Werror -std=c99 -O3 -o prime-slow prime-slow.c

clean:
	rm prime || true
	rm prime-slow || true
