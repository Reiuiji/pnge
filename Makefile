
all:
	gcc -lm -lpng -lcrypto -std=c11 pnge.c -o pnge

install:
	cp pnge /usr/bin/pnge

clean: 
	rm pnge

