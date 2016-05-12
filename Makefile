CC=cc
CFLAGS=-O2 -Wall
PROGRAM=cakeibo
OBJS=cakeibo.o util.o date.o file.o

.PHONY: clean all
.SUFFIXES: .c .o

all: $(PROGRAM)

$(PROGRAM): $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@

.c.o:
	$(CC) $(CFLAGS) -c $<

clean:
	$(RM) *.o $(PROGRAM)

