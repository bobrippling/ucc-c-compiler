include ../src_config.mk
include ../../config.mk

OBJ = platform.o util.o alloc.o dynarray.o dynmap.o

BUILD_CC = ${CC} ${CFLAGS} -c -g -o $@ $<

all: ${OBJ}

alloc.o: alloc.c
	${BUILD_CC}

platform.o: platform.c
	${BUILD_CC} -DUCC_PLATFORM=PLATFORM_${UCC_CPU}

util.o: util.c
	${BUILD_CC}

dynarray.o: dynarray.c
	${BUILD_CC}
dynmap.o: dynmap.c
	${BUILD_CC}

clean:
	rm -f *.o

.PHONY: all clean

platform.o: platform.c ../../config.mk
