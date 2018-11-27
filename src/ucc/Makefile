include ../config.mk

# mkstemp
CFLAGS += -D_XOPEN_SOURCE=500

OBJ = ucc.o ucc_ext.o ucc_path.o str.o warning.o spec.o \
			../util/alloc.o ../util/dynarray.o \
			../util/platform.o ../util/tmpfile.o \
			../util/str.o ../util/io.o

OBJ_TEST = test.o str.o ../util/alloc.o ../util/dynarray.o

SRC = ${OBJ:.o=.c}

all: ucc tags check

ucc: ${OBJ}
	@echo link $@
	$Q${CC} -o $@ ${OBJ} ${LDFLAGS}

test: ${OBJ_TEST}
	@echo link $@
	$Q${CC} -o $@ ${OBJ_TEST} ${LDFLAGS}

check: test
	@echo test ucc
	$Q./test

clean:
	@echo clean ucc
	$Qrm -f ${OBJ} ucc

.PHONY: clean all
include ../Makefile.common
