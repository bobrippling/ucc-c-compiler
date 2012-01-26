include ../src_config.mk

LDFLAGS += -lm

OBJ = ../util/alloc.o ../util/util.o ../util/platform.o ../util/dynarray.o \
	str.o \
	cc1.o \
	tree.o \
	tokconv.o tokenise.o \
	parse.o typedef.o \
	fold.o sym_fold.o const.o \
	sym.o struct.o \
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

asm.o: asm.c ../util/util.h tree.h cc1.h sym.h asm.h ../util/platform.h \
  ../util/alloc.h struct.h
asm_op.o: asm_op.c ../util/util.h tree.h cc1.h gen_asm.h asm.h asm_op.h
cc1.o: cc1.c ../util/util.h tree.h tokenise.h parse.h fold.h gen_asm.h \
  gen_str.h sym.h sym_fold.h cc1.h
const.o: const.c ../util/util.h tree.h const.h sym.h cc1.h
fold.o: fold.c ../util/util.h tree.h cc1.h fold.h sym_fold.h sym.h \
  ../util/platform.h const.h asm.h ../util/alloc.h ../util/dynarray.h \
  struct.h fold_op.c
fold_op.o: fold_op.c
gen_asm.o: gen_asm.c ../util/util.h tree.h cc1.h macros.h asm.h \
  ../util/platform.h sym.h asm_op.h gen_asm.h const.h struct.h
gen_str.o: gen_str.c ../util/util.h tree.h macros.h sym.h cc1.h struct.h
parse.o: parse.c tokenise.h ../util/util.h tree.h parse.h ../util/alloc.h \
  tokconv.h sym.h cc1.h typedef.h ../util/dynarray.h struct.h \
  parse_type.c
parse_type.o: parse_type.c
str.o: str.c ../util/util.h
struct.o: struct.c ../util/util.h ../util/alloc.h ../util/dynarray.h \
  tree.h struct.h
sym.o: sym.c ../util/util.h tree.h sym.h ../util/alloc.h macros.h \
  ../util/dynarray.h
sym_fold.o: sym_fold.c ../util/util.h tree.h cc1.h sym.h sym_fold.h \
  ../util/platform.h
tokconv.o: tokconv.c tokenise.h ../util/util.h tree.h tokconv.h macros.h
tokenise.o: tokenise.c ../util/util.h tree.h tokenise.h ../util/alloc.h \
  str.h
tree.o: tree.c ../util/alloc.h ../util/util.h tree.h macros.h sym.h \
  ../util/platform.h struct.h
typedef.o: typedef.c ../util/util.h tree.h ../util/alloc.h typedef.h
