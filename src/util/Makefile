include ../config.mk

OBJ = platform.o util.o alloc.o dynarray.o dynmap.o where.o str.o \
			std.o escape.o math.o warn.o path.o tmpfile.o
SRC = $(OBJ:.o=.c)

BUILD_CC = ${CC} ${CFLAGS} -c -o $@ $<

all: ${OBJ} check

platform.o: platform.c
	${BUILD_CC} -DUCC_PLATFORM=PLATFORM_${ARCH}

%.o: %.c
	${BUILD_CC}

deps: Makefile.dep

OBJ_TEST = test.o path.o dynarray.o alloc.o dynmap.o math.o
test: ${OBJ_TEST}
	${CC} -o $@ ${OBJ_TEST} ${LDFLAGS}

check: test
	./test

Makefile.deps:
	${CC} -MM ${SRC} > $@

clean:
	rm -f *.o

.PHONY: all clean check

include Makefile.deps
