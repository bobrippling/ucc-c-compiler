include ../src_config.mk

OBJ = preproc.o main.o macro.o str.o \
			../util/util.o ../util/alloc.o \
			../util/platform.o ../util/dynarray.o

cpp: ${OBJ}
	${CC} ${CFLAGS} -o $@ ${OBJ}

clean:
	rm -f *.o cpp

.PHONY: clean

macro.o: macro.c ../util/alloc.h ../util/util.h ../util/dynarray.h str.h \
 macro.h preproc.h
main.o: main.c macro.h preproc.h
preproc.o: preproc.c ../util/util.h ../util/alloc.h ../util/dynarray.h \
 macro.h
str.o: str.c str.h ../util/alloc.h
