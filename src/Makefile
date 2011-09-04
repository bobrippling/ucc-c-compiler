CFLAGS  = -g -Wall -Wextra -pedantic -std=c99
LDFLAGS = -g -lm

include ../config.mk

cc1: \
		alloc.o str.o util.o platform.o \
		cc1.o \
		tree.o \
		tokconv.o tokenise.o \
		parse.o \
		fold.o \
		sym.o \
		gen_asm.o gen.o gen_str.o \
		asm.o asm_op.o
	${CC} ${LDFLAGS} -o $@ $^

clean:
	rm -f *.o cc1

test: test.o ../lib/*.o
	${LD} ${UCC_LDFLAGS} -o $@ $< ../lib/*.o

test.o: test.s
	${NASM} -f ${ARCH} -o test.o test.s

test.s: TEST.c cc1
	./cc1 $< > test.s

.PHONY: clean

alloc.o: alloc.c tree.h alloc.h util.h
asm.o: asm.c tree.h sym.h asm.h platform.h alloc.h
asm_op.o: asm_op.c tree.h gen_asm.h asm.h util.h asm_op.h
cc1.o: cc1.c tree.h tokenise.h util.h parse.h fold.h gen.h gen_asm.h \
 gen_str.h
fold.o: fold.c tree.h fold.h util.h sym.h platform.h asm.h
gen_asm.o: gen_asm.c tree.h macros.h asm.h platform.h sym.h asm_op.h \
 gen_asm.h
gen.o: gen.c tree.h gen_str.h gen_asm.h
gen_str.o: gen_str.c tree.h macros.h sym.h
parse.o: parse.c tokenise.h tree.h parse.h alloc.h tokconv.h util.h
platform.o: platform.c platform.h
str.o: str.c
sym.o: sym.c tree.h sym.h alloc.h
tokconv.o: tokconv.c tokenise.h tree.h tokconv.h util.h macros.h
tokenise.o: tokenise.c tree.h tokenise.h alloc.h util.h str.h
tree.o: tree.c alloc.h tree.h macros.h
util.o: util.c tree.h util.h alloc.h
