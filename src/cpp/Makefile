CFLAGS = -Wall -Wextra -pedantic
OBJ    = pp.o main.o utils.o

cpp: ${OBJ}
	${CC} ${CFLAGS} -o $@ ${OBJ}

clean:
	rm -f *.o cpp

.PHONY: clean
