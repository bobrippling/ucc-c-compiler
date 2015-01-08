OBJ = backend.o main.o mem.o

val: ${OBJ}
	${CC} -o $@ ${OBJ} ${LDFALGS}

clean:
	rm -f val ${OBJ}

.PHONY: clean
