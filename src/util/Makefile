include ../src_config.mk

OBJ = platform.o util.o alloc.o

BUILD_CC = ${CC} -c -o $@ $<

all: ${OBJ}

platform.o: platform.c
	${BUILD_CC}

util.o: util.c
	${BUILD_CC}

clean:
	rm -f *.o

.PHONY: all clean

