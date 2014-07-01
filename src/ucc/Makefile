include ../src_config.mk

# mkstemp
CFLAGS += -D_XOPEN_SOURCE=500

OBJ = ucc.o ucc_ext.o ucc_lib.o str.o \
			../util/alloc.o ../util/dynarray.o ../util/platform.o ../util/path.o

ucc: ${OBJ}
	${CC} -o $@ ${OBJ}

clean:
	rm -f *.o ucc

.PHONY: clean

str.o: str.c ../util/alloc.h ../util/compiler.h ../util/dynarray.h \
 ../util/dyn.h str.h
ucc.o: ucc.c cfg.h ucc.h ucc_ext.h ucc_lib.h ../util/alloc.h \
 ../util/compiler.h ../util/dynarray.h ../util/dyn.h ../util/util.h \
 ../util/where.h ../util/platform.h str.h
ucc_ext.o: ucc_ext.c cfg.h ucc_ext.h ucc.h ../util/alloc.h \
 ../util/compiler.h ../util/dynarray.h ../util/dyn.h str.h
ucc_lib.o: ucc_lib.c ucc.h ucc_ext.h ucc_lib.h cfg.h str.h \
 ../util/alloc.h ../util/compiler.h ../util/dynarray.h ../util/dyn.h
