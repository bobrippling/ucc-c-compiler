include ../config.mk

# mkstemp
CFLAGS += -D_XOPEN_SOURCE=500

OBJ = ucc.o ucc_ext.o str.o warning.o spec.o \
			../util/alloc.o ../util/dynarray.o \
			../util/platform.o ../util/tmpfile.o \
			../util/str.o ../util/io.o ../util/target.o

SRC = ${OBJ:.o=.c}

all: ucc tags

ucc: ${OBJ}
	@echo link $@
	$Q${CC} -o $@ ${OBJ} ${LDFLAGS}

clean:
	@echo clean ucc
	$Qrm -f ${OBJ} ucc

.PHONY: clean all

include ../Makefile.common
