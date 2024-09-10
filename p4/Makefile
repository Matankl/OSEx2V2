CC = gcc
CFLAGS = 
LDFLAGS = -lstdc++

all: p4

p4: mync.o
	$(CC) $(CFLAGS) $(LDFLAGS) mync.o -o p4

mync.o: mync.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f mync.o p4

