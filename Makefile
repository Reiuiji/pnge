
all:
	gcc -lm -lpng -std=c11 pnge.c -o pnge

clean: 
	rm pnge

