include ../src_config.mk

CFLAGS += -D_POSIX_C_SOURCE=200112L

OBJ = ucc.o ucc_ext.o ucc_lib.o ucc_path.o specs.o str.o \
			../util/alloc.o ../util/dynarray.o ../util/platform.o

ucc: ${OBJ}
	${CC} -o $@ ${OBJ}

clean:
	rm -f *.o ucc

deps: Makefile.dep

Makefile.dep:
	cc -MM ${OBJ:.o=.c} > Makefile.deps

.PHONY: clean deps

include Makefile.deps
