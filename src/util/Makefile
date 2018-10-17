include ../config.mk

OBJ = platform.o util.o alloc.o dynarray.o dynmap.o where.o str.o \
			std.o escape.o math.o warn.o path.o tmpfile.o io.o target.o

OBJ_TEST = test.o path.o dynarray.o alloc.o dynmap.o math.o

SRC = ${OBJ:.o=.c} ${OBJ_TEST:.o=.c}

all: ${OBJ} tags check

platform.o: \
	CFLAGS += -DUCC_PLATFORM=PLATFORM_${ARCH}

test: ${OBJ_TEST}
	@echo link $@
	$Q${CC} -o $@ ${OBJ_TEST} ${LDFLAGS}

check: test
	@echo test util
	$Q./test

clean:
	@echo clean util
	$Qrm -f ${OBJ} ${OBJ_TEST}

.PHONY: all clean check

include ../Makefile.common
