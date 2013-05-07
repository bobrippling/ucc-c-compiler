include ../src_config.mk

OBJ = ucc.o ucc_ext.o ucc_lib.o \
			../util/alloc.o ../util/dynarray.o ../util/platform.o

ucc: ${OBJ}
	${CC} -o $@ ${OBJ}

clean:
	rm -f *.o ucc

.PHONY: clean

ucc.o: ucc.c ucc.h ucc_ext.h ucc_lib.h ../util/alloc.h ../util/dynarray.h
ucc_ext.o: ucc_ext.c ucc_ext.h ucc.h ../util/alloc.h ../util/dynarray.h \
 cfg.h
ucc_lib.o: ucc_lib.c ucc.h ucc_ext.h ucc_lib.h ../util/alloc.h \
 ../util/dynarray.h
