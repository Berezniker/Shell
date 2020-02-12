CC = gcc
CFLAGS = -g -Wall
PROG = shell

all: $(PROG)

$(PROG): $(PROG).c list.o tree.o exec.o bckgrnd.o
	$(CC) $(CFLAGS) -o $(PROG) $^

list.o: list.c list.h
	$(CC) $(CFLAGS) -c $<

tree.o: tree.c tree.h
	$(CC) $(CFLAGS) -c $<

exec.o: exec.c exec.h
	$(CC) $(CFLAGS) -c $<

bckgrnd.o: bckgrnd.c bckgrnd.h
	$(CC) $(CFLAGS) -c $<

run: $(PROG)
	rlwrap ./$(PROG)

val: $(PROG)
	valgrind ./$(PROG)

clean:
	rm -f $(PROG) *.o