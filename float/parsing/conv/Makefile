CFLAGS = -g -std=c99 -Wall -Wextra -pedantic
LDFLAGS = -g

all: f2l f2s.o test

f2l: f2l.c
f2s.o: f2s.c

test: f2s.o test.o

clean:
	rm -f *.o test f2l
