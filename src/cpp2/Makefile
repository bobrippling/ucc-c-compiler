include ../src_config.mk

OBJ = preproc.o main.o macro.o str.o parse.o \
			../util/util.o ../util/alloc.o \
			../util/platform.o ../util/dynarray.o

cpp: ${OBJ}
	${CC} ${CFLAGS} -o $@ ${OBJ}

clean:
	rm -f *.o cpp

.PHONY: clean

