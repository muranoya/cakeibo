CC=cc
CFLAGS=-O2 -Wall
PROGRAM=cakeibo
OBJS=main.o hashmap.o string.o util.o

.PHONY: clean all
.SUFFIXES: .c .o

all: $(PROGRAM)

$(PROGRAM): $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@

.c.o:
	$(CC) $(CFLAGS) -c $<

clean:
	$(RM) *.o $(PROGRAM)

