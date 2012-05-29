include src_config.mk

all: cc1 cpp2 cc

cc: cc.c cc.h util/alloc.o util/dynarray.o
	${CC} ${CFLAGS} -o $@ cc.c util/alloc.o util/dynarray.o

cc1: util
	make -C cc1

cpp2: util
	make -C cpp2

util:
	make -C util

clean:
	rm -f cc *.o
	make -C cc1  clean
	make -C cpp2 clean
	make -C util clean

.PHONY: cc1 cpp2 util clean
