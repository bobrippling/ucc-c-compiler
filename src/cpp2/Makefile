include ../src_config.mk

OBJ = preproc.o main.o macro.o str.o parse.o \
			../util/util.o ../util/alloc.o \
			../util/platform.o ../util/dynarray.o

cpp: ${OBJ}
	${CC} ${CFLAGS} -o $@ ${OBJ}

clean:
	rm -f *.o cpp

.PHONY: clean

macro.o: macro.c macro.h parse.h ../util/alloc.h ../util/dynarray.h \
 ../util/util.h str.h main.h
main.o: main.c macro.h preproc.h ../util/util.h ../util/dynarray.h \
 ../util/alloc.h ../util/platform.h
parse.o: parse.c parse.h macro.h ../util/alloc.h ../util/dynarray.h \
 ../util/util.h preproc.h main.h str.h
preproc.o: preproc.c ../util/util.h ../util/alloc.h ../util/dynarray.h \
 macro.h parse.h main.h
str.o: str.c str.h ../util/alloc.h ../util/util.h macro.h
