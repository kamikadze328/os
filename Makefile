GCC := gcc
CFLAGS := -pedantic-errors -Wall -Werror -pthread -o

all: main

main: main.c
	$(GCC) $(CFLAGS) main main.c

threads: threads.c
	$(GCC) $(CFLAGS) main threads.c

clean:
	rm -f main f0 f1 f2 f3 f4 f5 f6 f7 f8 f9 f10
