include ../src_config.mk

OBJ    = pp.o main.o ../util/util.o ../util/alloc.o ../util/platform.o

cpp: ${OBJ}
	${CC} ${CFLAGS} -o $@ ${OBJ}

clean:
	rm -f *.o cpp

.PHONY: clean

main.o: main.c pp.h
pp.o: pp.c pp.h ../util/util.h ../util/alloc.h ../util/platform.h
