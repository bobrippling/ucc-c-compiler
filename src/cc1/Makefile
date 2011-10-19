CFLAGS  = -g -Wall -Wextra -pedantic -std=c99
LDFLAGS = -g -lm

OBJ = ../util/alloc.o ../util/util.o ../util/platform.o \
	str.o \
	cc1.o \
	tree.o \
	tokconv.o tokenise.o \
	parse.o \
	fold.o const.o \
	sym.o \
	gen_asm.o gen_str.o \
	asm.o asm_op.o

include ../../config.mk

cc1: ${OBJ}
	${CC} ${LDFLAGS} -o $@ ${OBJ}

clean:
	rm -f *.o cc1

test: test.o ../../lib/*.o
	${LD} ${UCC_LDFLAGS} -o $@ $^

test.o: test.s
	${UCC_NASM} -f ${UCC_ARCH} -o test.o test.s

test.s: TEST.c cc1
	./cc1 TEST.c > test.s

.PHONY: clean
