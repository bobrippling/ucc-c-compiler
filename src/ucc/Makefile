include ../src_config.mk

# mkstemp
CFLAGS += -D_XOPEN_SOURCE=500

OBJ = ucc.o ucc_ext.o ucc_lib.o ucc_path.o str.o \
			../util/alloc.o ../util/dynarray.o ../util/platform.o

ucc: ${OBJ}
	${CC} -o $@ ${OBJ} ${LDFLAGS}

clean:
	rm -f *.o ucc

deps: Makefile.dep

Makefile.dep:
	cc -MM ${OBJ:.o=.c} > Makefile.deps

.PHONY: clean deps

include Makefile.deps
