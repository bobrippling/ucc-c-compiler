CFLAGS = -Wall -Wextra -pedantic -std=c99

all: cc1 cpp cc

cc: cc.c

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
