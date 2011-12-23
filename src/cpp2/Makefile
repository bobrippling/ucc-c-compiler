include ../src_config.mk

OBJ = preproc.o main.o macro.o \
			../util/util.o ../util/alloc.o ../util/platform.o

cpp: ${OBJ}
	${CC} ${CFLAGS} -o $@ ${OBJ}

clean:
	rm -f *.o cpp

.PHONY: clean

main.o: main.c
preproc.o: preproc.c
