include ../config.mk

ARCH_OBJ = out/${ARCH}.o

LDFLAGS += -lm

OBJ = \
	../util/alloc.o ../util/util.o ../util/platform.o \
	../util/dynarray.o ../util/dynmap.o ../util/where.o \
	../util/str.o ../util/std.o ../util/escape.o ../util/warn.o \
	../util/math.o ../util/tmpfile.o \
	str.o num.o label.o strings.o \
	pass1.o cc1_out_ctx.o \
	decl.o attribute.o decl_init.o funcargs.o expr.o stmt.o \
	btype.o type.o type_is.o type_nav.o op.o c_types.o c_funcs.o \
	tokconv.o tokenise.o \
	parse_expr.o parse_stmt.o parse_type.o parse_attr.o parse_init.o \
	fold.o fold_sym.o fold_sue.o const.o format_chk.o \
	sym.o sue.o ops/__builtin.o ops/__builtin_va.o pack.o vla.o \
	gen_asm.o gen_dump.o gen_style.o inline.o sanitize.o mangle.o \
	out/out.o out/asm.o out/lbl.o out/impl.o out/write.o out/dbg.o out/leb.o \
	out/virt.o out/ctrl.o out/func.o out/new.o out/val.o out/blk.o out/op.o \
	out/bitfield.o out/free.o out/alloca.o out/stack.o out/dbg_lbl.o out/mem.o \
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

OBJ_TEST = test.o ${OBJ}

SRC = ${OBJ:.o=.c}

all: cc1 check

cc1: ${OBJ} cc1.o
	${CC} -o $@ ${OBJ} cc1.o ${LDFLAGS}

test: ${OBJ_TEST}
	${CC} -o $@ ${OBJ_TEST} ${LDFLAGS}

check: test
	./test

%.o: %.c
	${CC} ${CFLAGS} -c -o $@ $<

clean:
	rm -f *.o cc1 ops/*.o out/*.o

tags: ${SRC}
	ctags -R .

.PHONY: clean deps

../util/alloc.o ../util/util.o ../util/platform.o ../util/dynarray.o \
../util/dynmap.o:
	@echo util must be built from util/
	false

# files that need target info
out/alloca.o out/ctrl.o out/func.o out/impl.o out/mipsel_32.o \
	out/new.o out/op.o out/out.o out/val.o out/virt.o out/vm.o \
	out/x86_64.o out/dbg.o : \
	CFLAGS += -DCC1_IMPL_FNAME=\"${ARCH}.h\"

# extra deps
out/impl.h: out/x86_64.h out/mipsel_32.h

deps: Makefile.dep

Makefile.dep: \
	CFLAGS += -DCC1_IMPL_FNAME=\"${ARCH}.h\"

Makefile.dep:
	${CC} ${CFLAGS} -MM *.c > $@
	${CC} ${CFLAGS} -MM ops/*.c | sed 's#^\([^ ]\)#ops/\1#' >> $@
	${CC} ${CFLAGS} -MM out/*.c | sed 's#^\([^ ]\)#out/\1#' >> $@

include Makefile.dep
