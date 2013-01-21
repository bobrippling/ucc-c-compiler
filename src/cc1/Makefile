include ../src_config.mk

# default to x86-64 for now

CFLAGS += -DCC1_IMPL_FNAME=\"x86_64.h\"
ARCH_OBJ = out/x86_64.o
#CFLAGS += -DCC1_IMPL_FNAME=\"mipsel_32.h\"
#ARCH_OBJ = out/mipsel_32.o


LDFLAGS += -lm

OBJ = \
	../util/alloc.o ../util/util.o ../util/platform.o \
	../util/dynarray.o ../util/dynmap.o \
	str.o \
	cc1.o \
	tree.o decl.o decl_init.o funcargs.o expr.o stmt.o \
	tokconv.o tokenise.o \
	parse.o parse_type.o typedef.o \
	fold.o fold_sym.o const.o \
	sym.o sue.o ops/__builtin.o pack.o \
	gen_asm.o gen_str.o gen_style.o \
	out/out.o out/asm.o out/lbl.o out/impl.c \
	ops/expr_addr.o ops/expr_assign.o ops/expr_cast.o ops/expr_comma.o \
	ops/expr_funcall.o ops/expr_identifier.o ops/expr_if.o ops/expr_op.o \
	ops/expr_sizeof.o ops/expr_val.o ops/expr_stmt.o ops/expr__Generic.o \
	ops/expr_deref.o ops/expr_struct.o ops/expr_compound_lit.o \
	ops/stmt_break.o ops/stmt_case.o ops/stmt_case_range.o ops/stmt_code.o \
	ops/stmt_default.o ops/stmt_do.o ops/stmt_expr.o ops/stmt_for.o ops/stmt_goto.o \
	ops/stmt_if.o ops/stmt_label.o ops/stmt_noop.o ops/stmt_return.o \
	ops/stmt_switch.o ops/stmt_while.o ops/stmt_continue.o ops/expr_block.o \
	ops/expr_assign_compound.o ops/expr_string.o \
	${ARCH_OBJ}

include ../../config.mk

cc1: ${OBJ}
	${CC} ${CFLAGS} ${LDFLAGS} -o $@ ${OBJ}

.c.o:
	${CC} ${CFLAGS} -c -o $@ $<

clean:
	rm -f *.o cc1 ops/*.o out/*.o

.PHONY: clean

include Makefile.dep
