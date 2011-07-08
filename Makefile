CFLAGS  = -g -Wall -Wextra -pedantic
LDFLAGS = -g -lm

cc1: tokenise.o parse.o cc1.o alloc.o util.o str.o tokconv.o
	${CC} ${LDFLAGS} -o $@ $^

clean:
	rm -f *.o cc1

test: cc1
	./cc1 INPUT.c

.PHONY: clean test
