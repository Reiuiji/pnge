
all:
	gcc -lm -lpng -lcrypto -std=c11 pnge.c -o pnge

clean: 
	rm pnge

