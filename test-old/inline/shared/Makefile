CC = ../../../ucc
OBJ = 2.o impl.o

prog: ${OBJ}
	${CC} -o $@ ${OBJ}

2.o: 2.c header.h
impl.o: impl.c header.h

clean:
	rm -f *.o prog

.PHONY: clean
