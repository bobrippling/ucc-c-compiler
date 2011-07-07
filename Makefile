CFLAGS  = -g -Wall -Wextra -pedantic
LDFLAGS = -g -lm

cc1: tokenise.o parse.o cc1.o util.o alloc.o str.o
	${CC} ${LDFLAGS} -o $@ $^

clean:
	rm -f *.o cc1

.PHONY: clean
