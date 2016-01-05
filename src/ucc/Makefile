include ../config.mk

# mkstemp
CFLAGS += -D_XOPEN_SOURCE=500

OBJ = ucc.o ucc_ext.o ucc_lib.o str.o warning.o \
			../util/alloc.o ../util/dynarray.o \
			../util/platform.o ../util/tmpfile.o

SRC = ${OBJ:.o=.c}

all: ucc tags

ucc: ${OBJ}
	${CC} -o $@ ${OBJ} ${LDFLAGS}

clean:
	rm -f ${OBJ} ucc

.PHONY: clean all

include ../Makefile.common
