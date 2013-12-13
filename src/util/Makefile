include ../src_config.mk
include ../../config.mk

OBJ = platform.o util.o alloc.o dynarray.o dynmap.o where.o str.o \
			std.o escape.o
SRC = $(OBJ:.o=.c)

BUILD_CC = ${CC} ${CFLAGS} -c -o $@ $<

all: ${OBJ}

alloc.o: alloc.c
	${BUILD_CC}

platform.o: platform.c
	${BUILD_CC} -DUCC_PLATFORM=PLATFORM_${UCC_CPU}

util.o: util.c
	${BUILD_CC}
where.o: where.c
	${BUILD_CC}
str.o: str.c
	${BUILD_CC}
std.o: std.c
	${BUILD_CC}
escape.o: escape.c
	${BUILD_CC}

dynarray.o: dynarray.c
	${BUILD_CC}
dynmap.o: dynmap.c
	${BUILD_CC}

deps: Makefile.dep

Makefile.deps:
	cc -MM ${SRC} > $@

clean:
	rm -f *.o

.PHONY: all clean

platform.o: platform.c ../../config.mk

-include Makefile.deps
