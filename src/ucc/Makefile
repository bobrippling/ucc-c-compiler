include ../config.mk

# mkstemp
CFLAGS += -D_XOPEN_SOURCE=500

OBJ_UCC = ucc.o ucc_ext.o str.o warning.o
OBJ_DEP = ../util/alloc.o ../util/dynarray.o \
		../util/platform.o ../util/tmpfile.o \
		../util/str.o ../util/io.o ../util/triple.o

OBJ = ${OBJ_UCC} ${OBJ_DEP}

SRC = ${OBJ_UCC:.o=.c}

all: ucc tags

ucc: ${OBJ}
	@echo link $@
	$Q${CC} -o $@ ${OBJ} ${LDFLAGS}

clean:
	@echo clean ucc
	$Qrm -f ${OBJ_UCC} ucc

.PHONY: clean all

include ../Makefile.common
