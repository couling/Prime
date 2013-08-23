all: prime prime-slow

prime: prime.c
	gcc -Werror -std=c99 -O3 -s -o prime prime.c 

prime-slow: prime-slow.c
	gcc -Werror -std=c99 -O3 -o prime-slow prime-slow.c

