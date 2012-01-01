include ../src_config.mk

OBJ    = pp.o main.o str.o \
				 ../util/util.o ../util/alloc.o \
				 ../util/platform.o ../util/dynarray.o

cpp: ${OBJ}
	${CC} ${CFLAGS} -o $@ ${OBJ}

clean:
	rm -f *.o cpp

.PHONY: clean

main.o: main.c pp.h ../util/alloc.h ../util/platform.h
pp.o: pp.c pp.h ../util/util.h ../util/alloc.h ../util/platform.h \
 ../util/dynarray.h str.h
str.o: str.c str.h ../util/alloc.h
