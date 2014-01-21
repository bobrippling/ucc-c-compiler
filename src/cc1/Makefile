include ../src_config.mk
include ../../config.mk

CPPFLAGS = -DCC1_IMPL_FNAME=\"${UCC_CPU}.h\"

ARCH_OBJ = out/${UCC_CPU}.o

CFLAGS  += -Wno-missing-field-initializers
LDFLAGS += -lm

OBJ = \
	../util/alloc.o ../util/util.o ../util/platform.o \
	../util/dynarray.o ../util/dynmap.o ../util/where.o \
	../util/str.o ../util/std.o ../util/escape.o \
	str.o num.o label.o strings.o \
	cc1.o pass1.o \
	decl.o attribute.o decl_init.o funcargs.o expr.o stmt.o \
	btype.o type.o type_is.o type_nav.o op.o c_types.o \
	tokconv.o tokenise.o \
	parse.o parse_type.o \
	fold.o fold_sym.o fold_sue.o const.o format_chk.o \
	sym.o sue.o ops/__builtin.o ops/__builtin_va.o pack.o \
	gen_asm.o gen_str.o gen_style.o \
	out/out.o out/asm.o out/lbl.o out/impl.o out/write.o out/dbg.o out/leb.o \
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

cc1: ${OBJ}
	${CC} ${CFLAGS} ${LDFLAGS} -o $@ ${OBJ}

.c.o:
	${CC} ${CFLAGS} -c -o $@ $<

clean:
	rm -f *.o cc1 ops/*.o out/*.o

tags:
	ctags -R .

.PHONY: clean deps

../util/alloc.o ../util/util.o ../util/platform.o ../util/dynarray.o \
../util/dynmap.o:
	@echo util must be built from util/
	false

# files that need target info
out/mipsel_32.o: out/mipsel_32.c
	${CC} ${CFLAGS} ${CPPFLAGS} -c -o $@ $<
out/x86_64.o: out/x86_64.c
	${CC} ${CFLAGS} ${CPPFLAGS} -c -o $@ $<
out/out.o: out/out.c out/impl.h
	${CC} ${CFLAGS} ${CPPFLAGS} -c -o $@ $<
out/impl.o: out/impl.c out/impl.h
	${CC} ${CFLAGS} ${CPPFLAGS} -c -o $@ $<

# extra deps
out/impl.h: out/x86_64.h out/mipsel_32.h

deps: Makefile.dep

Makefile.dep:
	cc ${CPPFLAGS} -MM *.c > $@
	cc ${CPPFLAGS} -MM ops/*.c | sed 's#^\([^ ]\)#ops/\1#' >> $@
	cc ${CPPFLAGS} -MM out/*.c | sed 's#^\([^ ]\)#out/\1#' >> $@

include Makefile.dep
