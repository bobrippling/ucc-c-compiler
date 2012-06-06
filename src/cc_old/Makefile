include ../src_config.mk

OBJ = cc.o ../util/alloc.o ../util/dynarray.o

cc: ${OBJ}
	${CC} ${CFLAGS} -o $@ ${OBJ}

cc.o: cc.c cc.h ../util/alloc.h ../util/dynarray.h
