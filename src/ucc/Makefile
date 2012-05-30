include ../src_config.mk

OBJ = ucc.o ucc_ext.o ucc_lib.o ../util/alloc.o ../util/dynarray.o

ucc: ${OBJ}
	${CC} -o $@ ${OBJ}

clean:
	rm -f *.o ucc

.PHONY: clean
