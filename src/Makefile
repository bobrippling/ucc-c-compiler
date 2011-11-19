include src_config.mk

all: cc1 cpp cc

cc: cc.c cc.h util/alloc.o
	${CC} ${CFLAGS} -o $@ cc.c util/alloc.o

cc1: util
	make -C cc1

cpp: util
	make -C cpp

util:
	make -C util

clean:
	rm -f cc *.o
	make -C cc1  clean
	make -C cpp  clean
	make -C util clean

.PHONY: cc1 cpp util clean
