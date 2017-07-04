all: dtparser

dtparser: main.o dtparser.o
	gcc -ggdb -Wall main.o dtparser.o -o dtparser

main.o: main.c
	gcc -ggdb -Wall -c main.c

dtparser.o: dtparser.c dtparser.h
	gcc -ggdb -Wall -c dtparser.c

clean:
	rm -rf *.o dtparser

.PHONY: clean

