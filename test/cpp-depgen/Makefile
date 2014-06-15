.PHONY: test clean

UCC ?= ../../ucc

test:
	deps=`${UCC} -ID -MM tim.c`; test "$$deps" = 'tim.o: tim.c stdio.h hi.h D/yo.h'

clean:
