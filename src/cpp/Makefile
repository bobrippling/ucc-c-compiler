CFLAGS = -Wall -Wextra -pedantic
OBJ    = pp.o main.o ../util/util.o ../util/alloc.o ../util/platform.o

cpp: ${OBJ}
	${CC} ${CFLAGS} -o $@ ${OBJ}

clean:
	rm -f *.o cpp

.PHONY: clean
