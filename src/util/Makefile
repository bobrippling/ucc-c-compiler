include ../src_config.mk

OBJ = platform.o util.o alloc.o

BUILD_CC = ${CC} -c -g -o $@ $<

all: ${OBJ}

alloc.o: alloc.c
	${BUILD_CC}

platform.o: platform.c
	${BUILD_CC}

util.o: util.c
	${BUILD_CC}

clean:
	rm -f *.o

.PHONY: all clean

