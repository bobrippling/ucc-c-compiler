include ../config.mk

LDFLAGS += -lm

OBJ_ARCH = out/${ARCH}.o

OBJ_UTIL = \
	../util/alloc.o ../util/util.o ../util/io.o ../util/platform.o \
	../util/dynarray.o ../util/dynmap.o ../util/where.o \
	../util/str.o ../util/std.o ../util/escape.o ../util/warn.o \
	../util/math.o ../util/tmpfile.o ../util/triple.o

OBJ_REST = \
	str.o num.o label.o strings.o \
	warn.o fopt.o visibility.o pass1.o cc1_out_ctx.o target.o \
	decl.o attribute.o decl_init.o funcargs.o expr.o stmt.o \
	btype.o type.o type_is.o type_nav.o op.o c_types.o c_funcs.o \
	tokconv.o tokenise.o pragma.o \
	parse_expr.o parse_stmt.o parse_type.o parse_attr.o parse_init.o \
	fold.o fold_sym.o fold_sue.o const.o format_chk.o \
	sym.o sue.o ops/__builtin.o ops/__builtin_va.o pack.o vla.o \
	gen_asm.o gen_dump.o gen_style.o gen_asm_ctors.o inline.o sanitize.o mangle.o \
	out/out.o out/asm.o out/lbl.o out/impl.o out/write.o out/dbg.o out/leb.o \
	out/virt.o out/ctrl.o out/func.o out/new.o out/val.o out/blk.o out/op.o \
	out/bitfield.o out/free.o out/alloca.o out/stack.o out/dbg_lbl.o out/mem.o \
	out/stack_protector.o \
	ops/expr_addr.o ops/expr_assign.o ops/expr_cast.o ops/expr_comma.o \
	ops/expr_funcall.o ops/expr_identifier.o ops/expr_if.o ops/expr_op.o \
	ops/expr_sizeof.o ops/expr_val.o ops/expr_stmt.o ops/expr__Generic.o \
	ops/expr_deref.o ops/expr_struct.o ops/expr_compound_lit.o \
	ops/stmt_break.o ops/stmt_case.o ops/stmt_case_range.o ops/stmt_code.o \
	ops/stmt_default.o ops/stmt_do.o ops/stmt_expr.o ops/stmt_for.o ops/stmt_goto.o \
	ops/stmt_if.o ops/stmt_label.o ops/stmt_noop.o ops/stmt_return.o \
	ops/stmt_switch.o ops/stmt_while.o ops/stmt_continue.o ops/expr_block.o \
	ops/expr_assign_compound.o ops/expr_string.o \
	${OBJ_ARCH}

BACKEND_SRC = \
		out/alloca.c out/ctrl.c out/func.c out/impl.c out/mipsel_32.c \
		out/new.c out/op.c out/out.c out/val.c out/virt.c out/vm.c \
		out/x86_64.c out/dbg.c out/stack_protector.c

OBJ_TEST = test.o ${OBJ_REST} ${OBJ_UTIL}
OBJ_CC1 = cc1.o ${OBJ_REST} ${OBJ_UTIL}

all: cc1 tags check

cc1: ${OBJ_CC1}
	@echo link $@
	$Q${CC} -o $@ ${OBJ_CC1} ${LDFLAGS}

test: ${OBJ_TEST}
	@echo link $@
	$Q${CC} -o $@ ${OBJ_TEST} ${LDFLAGS}

check: test
	@echo test cc1
	$Q./test

clean:
	@echo clean cc1
	$Qrm -f cc1 test ${OBJ_CC1} ${OBJ_TEST}

.PHONY: clean check

${OBJ_UTIL}:
	@echo util must be built from util/
	false

# files that need target info
${BACKEND_SRC:.c=.o} : \
	CFLAGS += -DCC1_IMPL_FNAME=\"${ARCH}.h\"

${BACKEND_SRC:.c=.d} : \
	CPPFLAGS += -DCC1_IMPL_FNAME=\"${ARCH}.h\"

# extra deps
out/impl.h: out/x86_64.h out/mipsel_32.h

SRC = cc1.c test.c ${OBJ_REST:.o=.c} # not util - we don't want dependencies for util/
include ../Makefile.common
