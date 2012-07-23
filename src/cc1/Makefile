include ../src_config.mk

CFLAGS += -Werror
LDFLAGS += -lm

OBJ = \
	../util/alloc.o ../util/util.o ../util/platform.o \
	../util/dynarray.o ../util/dynmap.o \
	str.o \
	cc1.o \
	tree.o decl.o expr.o stmt.o \
	tokconv.o tokenise.o \
	parse.o parse_type.o typedef.o \
	fold.o fold_sym.o const.o \
	sym.o sue.o namespace.o ops/__builtin.o \
	gen_asm.o asm.o gen_str.o gen_style.o \
	ops/expr_addr.o ops/expr_assign.o ops/expr_cast.o ops/expr_comma.o \
	ops/expr_funcall.o ops/expr_identifier.o ops/expr_if.o ops/expr_op.o \
	ops/expr_sizeof.o ops/expr_val.o ops/expr_stmt.o ops/expr__Generic.o \
	ops/stmt_break.o ops/stmt_case.o ops/stmt_case_range.o ops/stmt_code.o \
	ops/stmt_default.o ops/stmt_do.o ops/stmt_expr.o ops/stmt_for.o ops/stmt_goto.o \
	ops/stmt_if.o ops/stmt_label.o ops/stmt_noop.o ops/stmt_return.o \
	ops/stmt_switch.o ops/stmt_while.o ops/stmt_continue.o ops/expr_block.o

include ../../config.mk

cc1: ${OBJ}
	${CC} ${CFLAGS} ${LDFLAGS} -o $@ ${OBJ}

.c.o:
	${CC} ${CFLAGS} -c -o $@ $<

clean:
	rm -f *.o cc1 ops/*.o

test: test.o ../../lib/*.o
	${LD} ${UCC_LDFLAGS} -o $@ $^

test.o: test.s
	${UCC_NASM} -f ${UCC_ARCH} -o test.o test.s

test.s: TEST.c cc1
	./cc1 TEST.c > test.s

.PHONY: clean

include Makefile.dep
