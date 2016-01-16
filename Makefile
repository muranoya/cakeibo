CC=cc
CFLAGS=-O2 -Wall

all: cakeibo

cakeibo: main.o
	$(CC) $(CFLAGS) $^ -o $@

main.o: main.c
	$(CC) $(CFLAGS) $^ -c

.PHONY: clean all

clean:
	$(RM) *.o cakeibo

