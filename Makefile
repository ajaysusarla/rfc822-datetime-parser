all: rfc822dtparser

rfc822dtparser: main.o dtparser.o
	gcc -ggdb -Wall main.o dtparser.o -o rfc822dtparser

main.o: main.c
	gcc -ggdb -Wall -c main.c

dtparser.o: dtparser.c dtparser.h
	gcc -ggdb -Wall -c dtparser.c

clean:
	rm -rf *.o rfc822dtparser

check-syntax:
	$(CC) $(CFLAGS) -Wextra -pedantic -fsyntax-only $(CHK_SOURCES)

.PHONY: clean check-syntax
