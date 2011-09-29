CFLAGS = -Wall -Wextra -pedantic -std=c99

all: cc1 cpp cc

cc: cc.c

cc1:
	make -C cc1

cpp:
	make -C cpp

clean:
	make -C cc1 clean
	make -C cpp clean

.PHONY: cc1 cpp clean
