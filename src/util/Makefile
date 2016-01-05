include ../config.mk

OBJ = platform.o util.o alloc.o dynarray.o dynmap.o where.o str.o \
			std.o escape.o math.o warn.o path.o tmpfile.o

OBJ_TEST = test.o path.o dynarray.o alloc.o dynmap.o

SRC = ${OBJ:.o=.c} ${OBJ_TEST:.o=.c}

all: ${OBJ} tags check

platform.o: \
	CFLAGS += -DUCC_PLATFORM=PLATFORM_${ARCH}

test: ${OBJ_TEST}
	${CC} -o $@ ${OBJ_TEST} ${LDFLAGS}

check: test
	./test

clean:
	rm -f ${OBJ} ${OBJ_TEST}

.PHONY: all clean check

include ../Makefile.common
