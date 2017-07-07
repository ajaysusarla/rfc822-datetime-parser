DEBUG = -ggdb -Wall -Werror

all: rfc822dtparser

rfc822dtparser: main.o dtparser.o
	gcc $(DEBUG) main.o dtparser.o -o rfc822dtparser

main.o: main.c
	gcc $(DEBUG) -c main.c

dtparser.o: dtparser.c dtparser.h
	gcc $(DEBUG) -c dtparser.c

clean:
	rm -rf *.o rfc822dtparser

check-syntax:
	$(CC) $(CFLAGS) -Wextra -pedantic -fsyntax-only $(CHK_SOURCES)

.PHONY: clean check-syntax
