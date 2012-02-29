include ../src_config.mk

LDFLAGS += -lm

OBJ = ../util/alloc.o ../util/util.o ../util/platform.o ../util/dynarray.o \
	str.o \
	cc1.o \
	tree.o expr.o \
	tokconv.o tokenise.o \
	parse.o parse_type.o typedef.o \
	fold.o fold_sym.o const.o \
	sym.o struct.o enum.o struct_enum.o \
	gen_asm.o gen_str.o \
	asm.o asm_op.o

include ../../config.mk

cc1: ${OBJ}
	${CC} ${CFLAGS} ${LDFLAGS} -o $@ ${OBJ}

clean:
	rm -f *.o cc1

test: test.o ../../lib/*.o
	${LD} ${UCC_LDFLAGS} -o $@ $^

test.o: test.s
	${UCC_NASM} -f ${UCC_ARCH} -o test.o test.s

test.s: TEST.c cc1
	./cc1 TEST.c > test.s

.PHONY: clean
