include ../src_config.mk

LDFLAGS += -lm

OBJ = ../util/alloc.o ../util/util.o ../util/platform.o ../util/dynarray.o \
	str.o \
	cc1.o \
	tree.o expr.o stmt.o \
	tokconv.o tokenise.o \
	parse.o parse_type.o typedef.o \
	fold.o fold_sym.o const.o \
	sym.o struct.o enum.o struct_enum.o \
	gen_asm.o gen_str.o asm.o asm_out.o gen_style.o \
	ops/expr_addr.o ops/expr_assign.o ops/expr_cast.o ops/expr_comma.o \
	ops/expr_funcall.o ops/expr_identifier.o ops/expr_if.o ops/expr_op.o \
	ops/expr_sizeof.o ops/expr_val.o ops/expr_stmt.o \
	ops/stmt_break.o ops/stmt_case.o ops/stmt_case_range.o ops/stmt_code.o \
	ops/stmt_default.o ops/stmt_do.o ops/stmt_expr.o ops/stmt_for.o ops/stmt_goto.o \
	ops/stmt_if.o ops/stmt_label.o ops/stmt_noop.o ops/stmt_return.o \
	ops/stmt_switch.o ops/stmt_while.o ops/stmt_continue.o

include ../../config.mk

cc1: ${OBJ}
	${CC} ${CFLAGS} ${LDFLAGS} -o $@ ${OBJ}

clean:
	rm -f *.o cc1 ops/*.o

test: test.o ../../lib/*.o
	${LD} ${UCC_LDFLAGS} -o $@ $^

test.o: test.s
	${UCC_AS} -f ${UCC_ARCH} -o test.o test.s

test.s: TEST.c cc1
	./cc1 TEST.c > test.s

.PHONY: clean

# :r!cc -MM *.c ops/*.c
# :.,$s/^\(stmt\|expr\)\(_[^:]\+\)/ops\/\1\2/

asm.o: asm.c ../util/util.h data_structs.h tree.h expr.h ops/expr_addr.h \
 ops/expr_assign.h ops/expr_cast.h ops/expr_comma.h ops/expr_funcall.h \
 ops/expr_identifier.h ops/expr_if.h ops/expr_op.h ops/expr_sizeof.h \
 ops/expr_val.h ops/expr_stmt.h stmt.h ops/stmt_break.h ops/stmt_case.h \
 ops/stmt_case_range.h ops/stmt_code.h ops/stmt_default.h ops/stmt_do.h \
 ops/stmt_expr.h ops/stmt_for.h ops/stmt_goto.h ops/stmt_if.h \
 ops/stmt_label.h ops/stmt_noop.h ops/stmt_return.h ops/stmt_switch.h \
 ops/stmt_while.h ops/stmt_continue.h sym.h cc1.h asm_out.h asm.h \
 ../util/platform.h ../util/alloc.h struct.h
asm_out.o: asm_out.c ../util/util.h ../util/dynarray.h ../util/alloc.h \
 data_structs.h tree.h expr.h ops/expr_addr.h ops/expr_assign.h \
 ops/expr_cast.h ops/expr_comma.h ops/expr_funcall.h \
 ops/expr_identifier.h ops/expr_if.h ops/expr_op.h ops/expr_sizeof.h \
 ops/expr_val.h ops/expr_stmt.h stmt.h ops/stmt_break.h ops/stmt_case.h \
 ops/stmt_case_range.h ops/stmt_code.h ops/stmt_default.h ops/stmt_do.h \
 ops/stmt_expr.h ops/stmt_for.h ops/stmt_goto.h ops/stmt_if.h \
 ops/stmt_label.h ops/stmt_noop.h ops/stmt_return.h ops/stmt_switch.h \
 ops/stmt_while.h ops/stmt_continue.h sym.h cc1.h asm_out.h asm.h \
 ../util/platform.h
cc1.o: cc1.c ../util/util.h data_structs.h tree.h expr.h ops/expr_addr.h \
 ops/expr_assign.h ops/expr_cast.h ops/expr_comma.h ops/expr_funcall.h \
 ops/expr_identifier.h ops/expr_if.h ops/expr_op.h ops/expr_sizeof.h \
 ops/expr_val.h ops/expr_stmt.h stmt.h ops/stmt_break.h ops/stmt_case.h \
 ops/stmt_case_range.h ops/stmt_code.h ops/stmt_default.h ops/stmt_do.h \
 ops/stmt_expr.h ops/stmt_for.h ops/stmt_goto.h ops/stmt_if.h \
 ops/stmt_label.h ops/stmt_noop.h ops/stmt_return.h ops/stmt_switch.h \
 ops/stmt_while.h ops/stmt_continue.h sym.h tokenise.h parse.h cc1.h \
 fold.h gen_asm.h gen_str.h gen_style.h fold_sym.h asm_out.h
const.o: const.c ../util/util.h data_structs.h tree.h expr.h \
 ops/expr_addr.h ops/expr_assign.h ops/expr_cast.h ops/expr_comma.h \
 ops/expr_funcall.h ops/expr_identifier.h ops/expr_if.h ops/expr_op.h \
 ops/expr_sizeof.h ops/expr_val.h ops/expr_stmt.h stmt.h ops/stmt_break.h \
 ops/stmt_case.h ops/stmt_case_range.h ops/stmt_code.h ops/stmt_default.h \
 ops/stmt_do.h ops/stmt_expr.h ops/stmt_for.h ops/stmt_goto.h \
 ops/stmt_if.h ops/stmt_label.h ops/stmt_noop.h ops/stmt_return.h \
 ops/stmt_switch.h ops/stmt_while.h ops/stmt_continue.h sym.h const.h \
 cc1.h
enum.o: enum.c ../util/util.h ../util/alloc.h ../util/dynarray.h \
 data_structs.h tree.h expr.h ops/expr_addr.h ops/expr_assign.h \
 ops/expr_cast.h ops/expr_comma.h ops/expr_funcall.h \
 ops/expr_identifier.h ops/expr_if.h ops/expr_op.h ops/expr_sizeof.h \
 ops/expr_val.h ops/expr_stmt.h stmt.h ops/stmt_break.h ops/stmt_case.h \
 ops/stmt_case_range.h ops/stmt_code.h ops/stmt_default.h ops/stmt_do.h \
 ops/stmt_expr.h ops/stmt_for.h ops/stmt_goto.h ops/stmt_if.h \
 ops/stmt_label.h ops/stmt_noop.h ops/stmt_return.h ops/stmt_switch.h \
 ops/stmt_while.h ops/stmt_continue.h sym.h enum.h struct_enum.h
expr.o: expr.c ../util/util.h ../util/alloc.h data_structs.h tree.h \
 expr.h ops/expr_addr.h ops/expr_assign.h ops/expr_cast.h \
 ops/expr_comma.h ops/expr_funcall.h ops/expr_identifier.h ops/expr_if.h \
 ops/expr_op.h ops/expr_sizeof.h ops/expr_val.h ops/expr_stmt.h stmt.h \
 ops/stmt_break.h ops/stmt_case.h ops/stmt_case_range.h ops/stmt_code.h \
 ops/stmt_default.h ops/stmt_do.h ops/stmt_expr.h ops/stmt_for.h \
 ops/stmt_goto.h ops/stmt_if.h ops/stmt_label.h ops/stmt_noop.h \
 ops/stmt_return.h ops/stmt_switch.h ops/stmt_while.h ops/stmt_continue.h \
 sym.h cc1.h
fold.o: fold.c ../util/util.h data_structs.h tree.h expr.h \
 ops/expr_addr.h ops/expr_assign.h ops/expr_cast.h ops/expr_comma.h \
 ops/expr_funcall.h ops/expr_identifier.h ops/expr_if.h ops/expr_op.h \
 ops/expr_sizeof.h ops/expr_val.h ops/expr_stmt.h stmt.h ops/stmt_break.h \
 ops/stmt_case.h ops/stmt_case_range.h ops/stmt_code.h ops/stmt_default.h \
 ops/stmt_do.h ops/stmt_expr.h ops/stmt_for.h ops/stmt_goto.h \
 ops/stmt_if.h ops/stmt_label.h ops/stmt_noop.h ops/stmt_return.h \
 ops/stmt_switch.h ops/stmt_while.h ops/stmt_continue.h sym.h cc1.h \
 fold.h fold_sym.h ../util/platform.h const.h ../util/alloc.h \
 ../util/dynarray.h struct.h enum.h struct_enum.h
fold_sym.o: fold_sym.c ../util/util.h data_structs.h tree.h expr.h \
 ops/expr_addr.h ops/expr_assign.h ops/expr_cast.h ops/expr_comma.h \
 ops/expr_funcall.h ops/expr_identifier.h ops/expr_if.h ops/expr_op.h \
 ops/expr_sizeof.h ops/expr_val.h ops/expr_stmt.h stmt.h ops/stmt_break.h \
 ops/stmt_case.h ops/stmt_case_range.h ops/stmt_code.h ops/stmt_default.h \
 ops/stmt_do.h ops/stmt_expr.h ops/stmt_for.h ops/stmt_goto.h \
 ops/stmt_if.h ops/stmt_label.h ops/stmt_noop.h ops/stmt_return.h \
 ops/stmt_switch.h ops/stmt_while.h ops/stmt_continue.h sym.h cc1.h \
 fold_sym.h ../util/platform.h
gen_asm.o: gen_asm.c ../util/util.h data_structs.h tree.h expr.h \
 ops/expr_addr.h ops/expr_assign.h ops/expr_cast.h ops/expr_comma.h \
 ops/expr_funcall.h ops/expr_identifier.h ops/expr_if.h ops/expr_op.h \
 ops/expr_sizeof.h ops/expr_val.h ops/expr_stmt.h stmt.h ops/stmt_break.h \
 ops/stmt_case.h ops/stmt_case_range.h ops/stmt_code.h ops/stmt_default.h \
 ops/stmt_do.h ops/stmt_expr.h ops/stmt_for.h ops/stmt_goto.h \
 ops/stmt_if.h ops/stmt_label.h ops/stmt_noop.h ops/stmt_return.h \
 ops/stmt_switch.h ops/stmt_while.h ops/stmt_continue.h sym.h cc1.h \
 macros.h asm_out.h asm.h ../util/platform.h gen_asm.h const.h struct.h
gen_c.o: gen_c.c
gen_str.o: gen_str.c ../util/util.h data_structs.h tree.h expr.h \
 ops/expr_addr.h ops/expr_assign.h ops/expr_cast.h ops/expr_comma.h \
 ops/expr_funcall.h ops/expr_identifier.h ops/expr_if.h ops/expr_op.h \
 ops/expr_sizeof.h ops/expr_val.h ops/expr_stmt.h stmt.h ops/stmt_break.h \
 ops/stmt_case.h ops/stmt_case_range.h ops/stmt_code.h ops/stmt_default.h \
 ops/stmt_do.h ops/stmt_expr.h ops/stmt_for.h ops/stmt_goto.h \
 ops/stmt_if.h ops/stmt_label.h ops/stmt_noop.h ops/stmt_return.h \
 ops/stmt_switch.h ops/stmt_while.h ops/stmt_continue.h sym.h macros.h \
 cc1.h struct.h enum.h gen_str.h
gen_style.o: gen_style.c ../util/util.h data_structs.h tree.h expr.h \
 ops/expr_addr.h ops/expr_assign.h ops/expr_cast.h ops/expr_comma.h \
 ops/expr_funcall.h ops/expr_identifier.h ops/expr_if.h ops/expr_op.h \
 ops/expr_sizeof.h ops/expr_val.h ops/expr_stmt.h stmt.h ops/stmt_break.h \
 ops/stmt_case.h ops/stmt_case_range.h ops/stmt_code.h ops/stmt_default.h \
 ops/stmt_do.h ops/stmt_expr.h ops/stmt_for.h ops/stmt_goto.h \
 ops/stmt_if.h ops/stmt_label.h ops/stmt_noop.h ops/stmt_return.h \
 ops/stmt_switch.h ops/stmt_while.h ops/stmt_continue.h sym.h cc1.h \
 gen_style.h
parse_attr.o: parse_attr.c
parse.o: parse.c ../util/util.h data_structs.h tree.h expr.h \
 ops/expr_addr.h ops/expr_assign.h ops/expr_cast.h ops/expr_comma.h \
 ops/expr_funcall.h ops/expr_identifier.h ops/expr_if.h ops/expr_op.h \
 ops/expr_sizeof.h ops/expr_val.h ops/expr_stmt.h stmt.h ops/stmt_break.h \
 ops/stmt_case.h ops/stmt_case_range.h ops/stmt_code.h ops/stmt_default.h \
 ops/stmt_do.h ops/stmt_expr.h ops/stmt_for.h ops/stmt_goto.h \
 ops/stmt_if.h ops/stmt_label.h ops/stmt_noop.h ops/stmt_return.h \
 ops/stmt_switch.h ops/stmt_while.h ops/stmt_continue.h sym.h tokenise.h \
 parse.h ../util/alloc.h tokconv.h cc1.h ../util/dynarray.h struct.h \
 parse_type.h
parse_type.o: parse_type.c ../util/util.h ../util/alloc.h \
 ../util/dynarray.h data_structs.h tree.h expr.h ops/expr_addr.h \
 ops/expr_assign.h ops/expr_cast.h ops/expr_comma.h ops/expr_funcall.h \
 ops/expr_identifier.h ops/expr_if.h ops/expr_op.h ops/expr_sizeof.h \
 ops/expr_val.h ops/expr_stmt.h stmt.h ops/stmt_break.h ops/stmt_case.h \
 ops/stmt_case_range.h ops/stmt_code.h ops/stmt_default.h ops/stmt_do.h \
 ops/stmt_expr.h ops/stmt_for.h ops/stmt_goto.h ops/stmt_if.h \
 ops/stmt_label.h ops/stmt_noop.h ops/stmt_return.h ops/stmt_switch.h \
 ops/stmt_while.h ops/stmt_continue.h sym.h typedef.h tokenise.h \
 tokconv.h struct.h enum.h cc1.h parse.h parse_type.h parse_attr.c
stmt.o: stmt.c ../util/util.h ../util/alloc.h data_structs.h tree.h \
 expr.h ops/expr_addr.h ops/expr_assign.h ops/expr_cast.h \
 ops/expr_comma.h ops/expr_funcall.h ops/expr_identifier.h ops/expr_if.h \
 ops/expr_op.h ops/expr_sizeof.h ops/expr_val.h ops/expr_stmt.h stmt.h \
 ops/stmt_break.h ops/stmt_case.h ops/stmt_case_range.h ops/stmt_code.h \
 ops/stmt_default.h ops/stmt_do.h ops/stmt_expr.h ops/stmt_for.h \
 ops/stmt_goto.h ops/stmt_if.h ops/stmt_label.h ops/stmt_noop.h \
 ops/stmt_return.h ops/stmt_switch.h ops/stmt_while.h ops/stmt_continue.h \
 sym.h
str.o: str.c ../util/util.h
struct.o: struct.c ../util/util.h ../util/alloc.h ../util/dynarray.h \
 data_structs.h tree.h expr.h ops/expr_addr.h ops/expr_assign.h \
 ops/expr_cast.h ops/expr_comma.h ops/expr_funcall.h \
 ops/expr_identifier.h ops/expr_if.h ops/expr_op.h ops/expr_sizeof.h \
 ops/expr_val.h ops/expr_stmt.h stmt.h ops/stmt_break.h ops/stmt_case.h \
 ops/stmt_case_range.h ops/stmt_code.h ops/stmt_default.h ops/stmt_do.h \
 ops/stmt_expr.h ops/stmt_for.h ops/stmt_goto.h ops/stmt_if.h \
 ops/stmt_label.h ops/stmt_noop.h ops/stmt_return.h ops/stmt_switch.h \
 ops/stmt_while.h ops/stmt_continue.h sym.h struct.h struct_enum.h
struct_enum.o: struct_enum.c ../util/alloc.h ../util/util.h \
 data_structs.h tree.h expr.h ops/expr_addr.h ops/expr_assign.h \
 ops/expr_cast.h ops/expr_comma.h ops/expr_funcall.h \
 ops/expr_identifier.h ops/expr_if.h ops/expr_op.h ops/expr_sizeof.h \
 ops/expr_val.h ops/expr_stmt.h stmt.h ops/stmt_break.h ops/stmt_case.h \
 ops/stmt_case_range.h ops/stmt_code.h ops/stmt_default.h ops/stmt_do.h \
 ops/stmt_expr.h ops/stmt_for.h ops/stmt_goto.h ops/stmt_if.h \
 ops/stmt_label.h ops/stmt_noop.h ops/stmt_return.h ops/stmt_switch.h \
 ops/stmt_while.h ops/stmt_continue.h sym.h struct.h enum.h struct_enum.h \
 cc1.h
sym.o: sym.c ../util/util.h data_structs.h tree.h expr.h ops/expr_addr.h \
 ops/expr_assign.h ops/expr_cast.h ops/expr_comma.h ops/expr_funcall.h \
 ops/expr_identifier.h ops/expr_if.h ops/expr_op.h ops/expr_sizeof.h \
 ops/expr_val.h ops/expr_stmt.h stmt.h ops/stmt_break.h ops/stmt_case.h \
 ops/stmt_case_range.h ops/stmt_code.h ops/stmt_default.h ops/stmt_do.h \
 ops/stmt_expr.h ops/stmt_for.h ops/stmt_goto.h ops/stmt_if.h \
 ops/stmt_label.h ops/stmt_noop.h ops/stmt_return.h ops/stmt_switch.h \
 ops/stmt_while.h ops/stmt_continue.h sym.h ../util/alloc.h macros.h \
 ../util/dynarray.h typedef.h
tokconv.o: tokconv.c ../util/util.h data_structs.h tree.h expr.h \
 ops/expr_addr.h ops/expr_assign.h ops/expr_cast.h ops/expr_comma.h \
 ops/expr_funcall.h ops/expr_identifier.h ops/expr_if.h ops/expr_op.h \
 ops/expr_sizeof.h ops/expr_val.h ops/expr_stmt.h stmt.h ops/stmt_break.h \
 ops/stmt_case.h ops/stmt_case_range.h ops/stmt_code.h ops/stmt_default.h \
 ops/stmt_do.h ops/stmt_expr.h ops/stmt_for.h ops/stmt_goto.h \
 ops/stmt_if.h ops/stmt_label.h ops/stmt_noop.h ops/stmt_return.h \
 ops/stmt_switch.h ops/stmt_while.h ops/stmt_continue.h sym.h tokenise.h \
 tokconv.h macros.h
tokenise.o: tokenise.c ../util/util.h data_structs.h tree.h expr.h \
 ops/expr_addr.h ops/expr_assign.h ops/expr_cast.h ops/expr_comma.h \
 ops/expr_funcall.h ops/expr_identifier.h ops/expr_if.h ops/expr_op.h \
 ops/expr_sizeof.h ops/expr_val.h ops/expr_stmt.h stmt.h ops/stmt_break.h \
 ops/stmt_case.h ops/stmt_case_range.h ops/stmt_code.h ops/stmt_default.h \
 ops/stmt_do.h ops/stmt_expr.h ops/stmt_for.h ops/stmt_goto.h \
 ops/stmt_if.h ops/stmt_label.h ops/stmt_noop.h ops/stmt_return.h \
 ops/stmt_switch.h ops/stmt_while.h ops/stmt_continue.h sym.h tokenise.h \
 ../util/alloc.h str.h
tree.o: tree.c ../util/alloc.h ../util/util.h data_structs.h tree.h \
 expr.h ops/expr_addr.h ops/expr_assign.h ops/expr_cast.h \
 ops/expr_comma.h ops/expr_funcall.h ops/expr_identifier.h ops/expr_if.h \
 ops/expr_op.h ops/expr_sizeof.h ops/expr_val.h ops/expr_stmt.h stmt.h \
 ops/stmt_break.h ops/stmt_case.h ops/stmt_case_range.h ops/stmt_code.h \
 ops/stmt_default.h ops/stmt_do.h ops/stmt_expr.h ops/stmt_for.h \
 ops/stmt_goto.h ops/stmt_if.h ops/stmt_label.h ops/stmt_noop.h \
 ops/stmt_return.h ops/stmt_switch.h ops/stmt_while.h ops/stmt_continue.h \
 sym.h macros.h ../util/platform.h struct.h enum.h
typedef.o: typedef.c ../util/util.h data_structs.h tree.h expr.h \
 ops/expr_addr.h ops/expr_assign.h ops/expr_cast.h ops/expr_comma.h \
 ops/expr_funcall.h ops/expr_identifier.h ops/expr_if.h ops/expr_op.h \
 ops/expr_sizeof.h ops/expr_val.h ops/expr_stmt.h stmt.h ops/stmt_break.h \
 ops/stmt_case.h ops/stmt_case_range.h ops/stmt_code.h ops/stmt_default.h \
 ops/stmt_do.h ops/stmt_expr.h ops/stmt_for.h ops/stmt_goto.h \
 ops/stmt_if.h ops/stmt_label.h ops/stmt_noop.h ops/stmt_return.h \
 ops/stmt_switch.h ops/stmt_while.h ops/stmt_continue.h sym.h \
 ../util/alloc.h typedef.h
ops/expr_addr.o: ops/expr_addr.c ops/ops.h ops/../../util/util.h \
 ops/../data_structs.h ops/../tree.h ops/../expr.h ops/../ops/expr_addr.h \
 ops/../ops/expr_assign.h ops/../ops/expr_cast.h ops/../ops/expr_comma.h \
 ops/../ops/expr_funcall.h ops/../ops/expr_identifier.h \
 ops/../ops/expr_if.h ops/../ops/expr_op.h ops/../ops/expr_sizeof.h \
 ops/../ops/expr_val.h ops/../ops/expr_stmt.h ops/../stmt.h \
 ops/../ops/stmt_break.h ops/../ops/stmt_case.h \
 ops/../ops/stmt_case_range.h ops/../ops/stmt_code.h \
 ops/../ops/stmt_default.h ops/../ops/stmt_do.h ops/../ops/stmt_expr.h \
 ops/../ops/stmt_for.h ops/../ops/stmt_goto.h ops/../ops/stmt_if.h \
 ops/../ops/stmt_label.h ops/../ops/stmt_noop.h ops/../ops/stmt_return.h \
 ops/../ops/stmt_switch.h ops/../ops/stmt_while.h \
 ops/../ops/stmt_continue.h ops/../sym.h ops/expr_addr.h \
 ops/expr_assign.h ops/expr_cast.h ops/expr_comma.h ops/expr_funcall.h \
 ops/expr_identifier.h ops/expr_if.h ops/expr_op.h ops/expr_sizeof.h \
 ops/expr_val.h ops/stmt_break.h ops/stmt_case.h ops/stmt_case_range.h \
 ops/stmt_code.h ops/stmt_default.h ops/stmt_do.h ops/stmt_expr.h \
 ops/stmt_for.h ops/stmt_goto.h ops/stmt_if.h ops/stmt_label.h \
 ops/stmt_noop.h ops/stmt_return.h ops/stmt_switch.h ops/stmt_while.h \
 ops/../cc1.h ops/../asm_out.h ops/../asm.h ops/../fold.h ops/../const.h \
 ops/../gen_str.h ops/../gen_asm.h ops/../struct.h
ops/expr_assign.o: ops/expr_assign.c ops/ops.h ops/../../util/util.h \
 ops/../data_structs.h ops/../tree.h ops/../expr.h ops/../ops/expr_addr.h \
 ops/../ops/expr_assign.h ops/../ops/expr_cast.h ops/../ops/expr_comma.h \
 ops/../ops/expr_funcall.h ops/../ops/expr_identifier.h \
 ops/../ops/expr_if.h ops/../ops/expr_op.h ops/../ops/expr_sizeof.h \
 ops/../ops/expr_val.h ops/../ops/expr_stmt.h ops/../stmt.h \
 ops/../ops/stmt_break.h ops/../ops/stmt_case.h \
 ops/../ops/stmt_case_range.h ops/../ops/stmt_code.h \
 ops/../ops/stmt_default.h ops/../ops/stmt_do.h ops/../ops/stmt_expr.h \
 ops/../ops/stmt_for.h ops/../ops/stmt_goto.h ops/../ops/stmt_if.h \
 ops/../ops/stmt_label.h ops/../ops/stmt_noop.h ops/../ops/stmt_return.h \
 ops/../ops/stmt_switch.h ops/../ops/stmt_while.h \
 ops/../ops/stmt_continue.h ops/../sym.h ops/expr_addr.h \
 ops/expr_assign.h ops/expr_cast.h ops/expr_comma.h ops/expr_funcall.h \
 ops/expr_identifier.h ops/expr_if.h ops/expr_op.h ops/expr_sizeof.h \
 ops/expr_val.h ops/stmt_break.h ops/stmt_case.h ops/stmt_case_range.h \
 ops/stmt_code.h ops/stmt_default.h ops/stmt_do.h ops/stmt_expr.h \
 ops/stmt_for.h ops/stmt_goto.h ops/stmt_if.h ops/stmt_label.h \
 ops/stmt_noop.h ops/stmt_return.h ops/stmt_switch.h ops/stmt_while.h \
 ops/../cc1.h ops/../asm_out.h ops/../asm.h ops/../fold.h ops/../const.h \
 ops/../gen_str.h ops/../gen_asm.h
ops/expr_cast.o: ops/expr_cast.c ops/../../util/alloc.h ops/ops.h \
 ops/../../util/util.h ops/../data_structs.h ops/../tree.h ops/../expr.h \
 ops/../ops/expr_addr.h ops/../ops/expr_assign.h ops/../ops/expr_cast.h \
 ops/../ops/expr_comma.h ops/../ops/expr_funcall.h \
 ops/../ops/expr_identifier.h ops/../ops/expr_if.h ops/../ops/expr_op.h \
 ops/../ops/expr_sizeof.h ops/../ops/expr_val.h ops/../ops/expr_stmt.h \
 ops/../stmt.h ops/../ops/stmt_break.h ops/../ops/stmt_case.h \
 ops/../ops/stmt_case_range.h ops/../ops/stmt_code.h \
 ops/../ops/stmt_default.h ops/../ops/stmt_do.h ops/../ops/stmt_expr.h \
 ops/../ops/stmt_for.h ops/../ops/stmt_goto.h ops/../ops/stmt_if.h \
 ops/../ops/stmt_label.h ops/../ops/stmt_noop.h ops/../ops/stmt_return.h \
 ops/../ops/stmt_switch.h ops/../ops/stmt_while.h \
 ops/../ops/stmt_continue.h ops/../sym.h ops/expr_addr.h \
 ops/expr_assign.h ops/expr_cast.h ops/expr_comma.h ops/expr_funcall.h \
 ops/expr_identifier.h ops/expr_if.h ops/expr_op.h ops/expr_sizeof.h \
 ops/expr_val.h ops/stmt_break.h ops/stmt_case.h ops/stmt_case_range.h \
 ops/stmt_code.h ops/stmt_default.h ops/stmt_do.h ops/stmt_expr.h \
 ops/stmt_for.h ops/stmt_goto.h ops/stmt_if.h ops/stmt_label.h \
 ops/stmt_noop.h ops/stmt_return.h ops/stmt_switch.h ops/stmt_while.h \
 ops/../cc1.h ops/../asm_out.h ops/../asm.h ops/../fold.h ops/../const.h \
 ops/../gen_str.h ops/../gen_asm.h
ops/expr_comma.o: ops/expr_comma.c ops/ops.h ops/../../util/util.h \
 ops/../data_structs.h ops/../tree.h ops/../expr.h ops/../ops/expr_addr.h \
 ops/../ops/expr_assign.h ops/../ops/expr_cast.h ops/../ops/expr_comma.h \
 ops/../ops/expr_funcall.h ops/../ops/expr_identifier.h \
 ops/../ops/expr_if.h ops/../ops/expr_op.h ops/../ops/expr_sizeof.h \
 ops/../ops/expr_val.h ops/../ops/expr_stmt.h ops/../stmt.h \
 ops/../ops/stmt_break.h ops/../ops/stmt_case.h \
 ops/../ops/stmt_case_range.h ops/../ops/stmt_code.h \
 ops/../ops/stmt_default.h ops/../ops/stmt_do.h ops/../ops/stmt_expr.h \
 ops/../ops/stmt_for.h ops/../ops/stmt_goto.h ops/../ops/stmt_if.h \
 ops/../ops/stmt_label.h ops/../ops/stmt_noop.h ops/../ops/stmt_return.h \
 ops/../ops/stmt_switch.h ops/../ops/stmt_while.h \
 ops/../ops/stmt_continue.h ops/../sym.h ops/expr_addr.h \
 ops/expr_assign.h ops/expr_cast.h ops/expr_comma.h ops/expr_funcall.h \
 ops/expr_identifier.h ops/expr_if.h ops/expr_op.h ops/expr_sizeof.h \
 ops/expr_val.h ops/stmt_break.h ops/stmt_case.h ops/stmt_case_range.h \
 ops/stmt_code.h ops/stmt_default.h ops/stmt_do.h ops/stmt_expr.h \
 ops/stmt_for.h ops/stmt_goto.h ops/stmt_if.h ops/stmt_label.h \
 ops/stmt_noop.h ops/stmt_return.h ops/stmt_switch.h ops/stmt_while.h \
 ops/../cc1.h ops/../asm_out.h ops/../asm.h ops/../fold.h ops/../const.h \
 ops/../gen_str.h ops/../gen_asm.h
ops/expr_funcall.o: ops/expr_funcall.c ops/ops.h ops/../../util/util.h \
 ops/../data_structs.h ops/../tree.h ops/../expr.h ops/../ops/expr_addr.h \
 ops/../ops/expr_assign.h ops/../ops/expr_cast.h ops/../ops/expr_comma.h \
 ops/../ops/expr_funcall.h ops/../ops/expr_identifier.h \
 ops/../ops/expr_if.h ops/../ops/expr_op.h ops/../ops/expr_sizeof.h \
 ops/../ops/expr_val.h ops/../ops/expr_stmt.h ops/../stmt.h \
 ops/../ops/stmt_break.h ops/../ops/stmt_case.h \
 ops/../ops/stmt_case_range.h ops/../ops/stmt_code.h \
 ops/../ops/stmt_default.h ops/../ops/stmt_do.h ops/../ops/stmt_expr.h \
 ops/../ops/stmt_for.h ops/../ops/stmt_goto.h ops/../ops/stmt_if.h \
 ops/../ops/stmt_label.h ops/../ops/stmt_noop.h ops/../ops/stmt_return.h \
 ops/../ops/stmt_switch.h ops/../ops/stmt_while.h \
 ops/../ops/stmt_continue.h ops/../sym.h ops/expr_addr.h \
 ops/expr_assign.h ops/expr_cast.h ops/expr_comma.h ops/expr_funcall.h \
 ops/expr_identifier.h ops/expr_if.h ops/expr_op.h ops/expr_sizeof.h \
 ops/expr_val.h ops/stmt_break.h ops/stmt_case.h ops/stmt_case_range.h \
 ops/stmt_code.h ops/stmt_default.h ops/stmt_do.h ops/stmt_expr.h \
 ops/stmt_for.h ops/stmt_goto.h ops/stmt_if.h ops/stmt_label.h \
 ops/stmt_noop.h ops/stmt_return.h ops/stmt_switch.h ops/stmt_while.h \
 ops/../cc1.h ops/../asm_out.h ops/../asm.h ops/../fold.h ops/../const.h \
 ops/../gen_str.h ops/../gen_asm.h ops/../../util/dynarray.h \
 ops/../../util/platform.h
ops/expr_identifier.o: ops/expr_identifier.c ops/ops.h ops/../../util/util.h \
 ops/../data_structs.h ops/../tree.h ops/../expr.h ops/../ops/expr_addr.h \
 ops/../ops/expr_assign.h ops/../ops/expr_cast.h ops/../ops/expr_comma.h \
 ops/../ops/expr_funcall.h ops/../ops/expr_identifier.h \
 ops/../ops/expr_if.h ops/../ops/expr_op.h ops/../ops/expr_sizeof.h \
 ops/../ops/expr_val.h ops/../ops/expr_stmt.h ops/../stmt.h \
 ops/../ops/stmt_break.h ops/../ops/stmt_case.h \
 ops/../ops/stmt_case_range.h ops/../ops/stmt_code.h \
 ops/../ops/stmt_default.h ops/../ops/stmt_do.h ops/../ops/stmt_expr.h \
 ops/../ops/stmt_for.h ops/../ops/stmt_goto.h ops/../ops/stmt_if.h \
 ops/../ops/stmt_label.h ops/../ops/stmt_noop.h ops/../ops/stmt_return.h \
 ops/../ops/stmt_switch.h ops/../ops/stmt_while.h \
 ops/../ops/stmt_continue.h ops/../sym.h ops/expr_addr.h \
 ops/expr_assign.h ops/expr_cast.h ops/expr_comma.h ops/expr_funcall.h \
 ops/expr_identifier.h ops/expr_if.h ops/expr_op.h ops/expr_sizeof.h \
 ops/expr_val.h ops/stmt_break.h ops/stmt_case.h ops/stmt_case_range.h \
 ops/stmt_code.h ops/stmt_default.h ops/stmt_do.h ops/stmt_expr.h \
 ops/stmt_for.h ops/stmt_goto.h ops/stmt_if.h ops/stmt_label.h \
 ops/stmt_noop.h ops/stmt_return.h ops/stmt_switch.h ops/stmt_while.h \
 ops/../cc1.h ops/../asm_out.h ops/../asm.h ops/../fold.h ops/../const.h \
 ops/../gen_str.h ops/../gen_asm.h ops/../enum.h
ops/expr_if.o: ops/expr_if.c ops/ops.h ops/../../util/util.h \
 ops/../data_structs.h ops/../tree.h ops/../expr.h ops/../ops/expr_addr.h \
 ops/../ops/expr_assign.h ops/../ops/expr_cast.h ops/../ops/expr_comma.h \
 ops/../ops/expr_funcall.h ops/../ops/expr_identifier.h \
 ops/../ops/expr_if.h ops/../ops/expr_op.h ops/../ops/expr_sizeof.h \
 ops/../ops/expr_val.h ops/../ops/expr_stmt.h ops/../stmt.h \
 ops/../ops/stmt_break.h ops/../ops/stmt_case.h \
 ops/../ops/stmt_case_range.h ops/../ops/stmt_code.h \
 ops/../ops/stmt_default.h ops/../ops/stmt_do.h ops/../ops/stmt_expr.h \
 ops/../ops/stmt_for.h ops/../ops/stmt_goto.h ops/../ops/stmt_if.h \
 ops/../ops/stmt_label.h ops/../ops/stmt_noop.h ops/../ops/stmt_return.h \
 ops/../ops/stmt_switch.h ops/../ops/stmt_while.h \
 ops/../ops/stmt_continue.h ops/../sym.h ops/expr_addr.h \
 ops/expr_assign.h ops/expr_cast.h ops/expr_comma.h ops/expr_funcall.h \
 ops/expr_identifier.h ops/expr_if.h ops/expr_op.h ops/expr_sizeof.h \
 ops/expr_val.h ops/stmt_break.h ops/stmt_case.h ops/stmt_case_range.h \
 ops/stmt_code.h ops/stmt_default.h ops/stmt_do.h ops/stmt_expr.h \
 ops/stmt_for.h ops/stmt_goto.h ops/stmt_if.h ops/stmt_label.h \
 ops/stmt_noop.h ops/stmt_return.h ops/stmt_switch.h ops/stmt_while.h \
 ops/../cc1.h ops/../asm_out.h ops/../asm.h ops/../fold.h ops/../const.h \
 ops/../gen_str.h ops/../gen_asm.h
ops/expr_op.o: ops/expr_op.c ops/ops.h ops/../../util/util.h \
 ops/../data_structs.h ops/../tree.h ops/../expr.h ops/../ops/expr_addr.h \
 ops/../ops/expr_assign.h ops/../ops/expr_cast.h ops/../ops/expr_comma.h \
 ops/../ops/expr_funcall.h ops/../ops/expr_identifier.h \
 ops/../ops/expr_if.h ops/../ops/expr_op.h ops/../ops/expr_sizeof.h \
 ops/../ops/expr_val.h ops/../ops/expr_stmt.h ops/../stmt.h \
 ops/../ops/stmt_break.h ops/../ops/stmt_case.h \
 ops/../ops/stmt_case_range.h ops/../ops/stmt_code.h \
 ops/../ops/stmt_default.h ops/../ops/stmt_do.h ops/../ops/stmt_expr.h \
 ops/../ops/stmt_for.h ops/../ops/stmt_goto.h ops/../ops/stmt_if.h \
 ops/../ops/stmt_label.h ops/../ops/stmt_noop.h ops/../ops/stmt_return.h \
 ops/../ops/stmt_switch.h ops/../ops/stmt_while.h \
 ops/../ops/stmt_continue.h ops/../sym.h ops/expr_addr.h \
 ops/expr_assign.h ops/expr_cast.h ops/expr_comma.h ops/expr_funcall.h \
 ops/expr_identifier.h ops/expr_if.h ops/expr_op.h ops/expr_sizeof.h \
 ops/expr_val.h ops/stmt_break.h ops/stmt_case.h ops/stmt_case_range.h \
 ops/stmt_code.h ops/stmt_default.h ops/stmt_do.h ops/stmt_expr.h \
 ops/stmt_for.h ops/stmt_goto.h ops/stmt_if.h ops/stmt_label.h \
 ops/stmt_noop.h ops/stmt_return.h ops/stmt_switch.h ops/stmt_while.h \
 ops/../cc1.h ops/../asm_out.h ops/../asm.h ops/../fold.h ops/../const.h \
 ops/../gen_str.h ops/../gen_asm.h ops/../struct.h
ops/expr_sizeof.o: ops/expr_sizeof.c ops/ops.h ops/../../util/util.h \
 ops/../data_structs.h ops/../tree.h ops/../expr.h ops/../ops/expr_addr.h \
 ops/../ops/expr_assign.h ops/../ops/expr_cast.h ops/../ops/expr_comma.h \
 ops/../ops/expr_funcall.h ops/../ops/expr_identifier.h \
 ops/../ops/expr_if.h ops/../ops/expr_op.h ops/../ops/expr_sizeof.h \
 ops/../ops/expr_val.h ops/../ops/expr_stmt.h ops/../stmt.h \
 ops/../ops/stmt_break.h ops/../ops/stmt_case.h \
 ops/../ops/stmt_case_range.h ops/../ops/stmt_code.h \
 ops/../ops/stmt_default.h ops/../ops/stmt_do.h ops/../ops/stmt_expr.h \
 ops/../ops/stmt_for.h ops/../ops/stmt_goto.h ops/../ops/stmt_if.h \
 ops/../ops/stmt_label.h ops/../ops/stmt_noop.h ops/../ops/stmt_return.h \
 ops/../ops/stmt_switch.h ops/../ops/stmt_while.h \
 ops/../ops/stmt_continue.h ops/../sym.h ops/expr_addr.h \
 ops/expr_assign.h ops/expr_cast.h ops/expr_comma.h ops/expr_funcall.h \
 ops/expr_identifier.h ops/expr_if.h ops/expr_op.h ops/expr_sizeof.h \
 ops/expr_val.h ops/stmt_break.h ops/stmt_case.h ops/stmt_case_range.h \
 ops/stmt_code.h ops/stmt_default.h ops/stmt_do.h ops/stmt_expr.h \
 ops/stmt_for.h ops/stmt_goto.h ops/stmt_if.h ops/stmt_label.h \
 ops/stmt_noop.h ops/stmt_return.h ops/stmt_switch.h ops/stmt_while.h \
 ops/../cc1.h ops/../asm_out.h ops/../asm.h ops/../fold.h ops/../const.h \
 ops/../gen_str.h ops/../gen_asm.h
ops/expr_stmt.o: ops/expr_stmt.c ops/ops.h ops/../../util/util.h \
 ops/../data_structs.h ops/../tree.h ops/../expr.h ops/../ops/expr_addr.h \
 ops/../ops/expr_assign.h ops/../ops/expr_cast.h ops/../ops/expr_comma.h \
 ops/../ops/expr_funcall.h ops/../ops/expr_identifier.h \
 ops/../ops/expr_if.h ops/../ops/expr_op.h ops/../ops/expr_sizeof.h \
 ops/../ops/expr_val.h ops/../ops/expr_stmt.h ops/../stmt.h \
 ops/../ops/stmt_break.h ops/../ops/stmt_case.h \
 ops/../ops/stmt_case_range.h ops/../ops/stmt_code.h \
 ops/../ops/stmt_default.h ops/../ops/stmt_do.h ops/../ops/stmt_expr.h \
 ops/../ops/stmt_for.h ops/../ops/stmt_goto.h ops/../ops/stmt_if.h \
 ops/../ops/stmt_label.h ops/../ops/stmt_noop.h ops/../ops/stmt_return.h \
 ops/../ops/stmt_switch.h ops/../ops/stmt_while.h \
 ops/../ops/stmt_continue.h ops/../sym.h ops/expr_addr.h \
 ops/expr_assign.h ops/expr_cast.h ops/expr_comma.h ops/expr_funcall.h \
 ops/expr_identifier.h ops/expr_if.h ops/expr_op.h ops/expr_sizeof.h \
 ops/expr_val.h ops/stmt_break.h ops/stmt_case.h ops/stmt_case_range.h \
 ops/stmt_code.h ops/stmt_default.h ops/stmt_do.h ops/stmt_expr.h \
 ops/stmt_for.h ops/stmt_goto.h ops/stmt_if.h ops/stmt_label.h \
 ops/stmt_noop.h ops/stmt_return.h ops/stmt_switch.h ops/stmt_while.h \
 ops/../cc1.h ops/../asm_out.h ops/../asm.h ops/../fold.h ops/../const.h \
 ops/../gen_str.h ops/../gen_asm.h ops/../../util/dynarray.h
ops/expr_val.o: ops/expr_val.c ops/ops.h ops/../../util/util.h \
 ops/../data_structs.h ops/../tree.h ops/../expr.h ops/../ops/expr_addr.h \
 ops/../ops/expr_assign.h ops/../ops/expr_cast.h ops/../ops/expr_comma.h \
 ops/../ops/expr_funcall.h ops/../ops/expr_identifier.h \
 ops/../ops/expr_if.h ops/../ops/expr_op.h ops/../ops/expr_sizeof.h \
 ops/../ops/expr_val.h ops/../ops/expr_stmt.h ops/../stmt.h \
 ops/../ops/stmt_break.h ops/../ops/stmt_case.h \
 ops/../ops/stmt_case_range.h ops/../ops/stmt_code.h \
 ops/../ops/stmt_default.h ops/../ops/stmt_do.h ops/../ops/stmt_expr.h \
 ops/../ops/stmt_for.h ops/../ops/stmt_goto.h ops/../ops/stmt_if.h \
 ops/../ops/stmt_label.h ops/../ops/stmt_noop.h ops/../ops/stmt_return.h \
 ops/../ops/stmt_switch.h ops/../ops/stmt_while.h \
 ops/../ops/stmt_continue.h ops/../sym.h ops/expr_addr.h \
 ops/expr_assign.h ops/expr_cast.h ops/expr_comma.h ops/expr_funcall.h \
 ops/expr_identifier.h ops/expr_if.h ops/expr_op.h ops/expr_sizeof.h \
 ops/expr_val.h ops/stmt_break.h ops/stmt_case.h ops/stmt_case_range.h \
 ops/stmt_code.h ops/stmt_default.h ops/stmt_do.h ops/stmt_expr.h \
 ops/stmt_for.h ops/stmt_goto.h ops/stmt_if.h ops/stmt_label.h \
 ops/stmt_noop.h ops/stmt_return.h ops/stmt_switch.h ops/stmt_while.h \
 ops/../cc1.h ops/../asm_out.h ops/../asm.h ops/../fold.h ops/../const.h \
 ops/../gen_str.h ops/../gen_asm.h
ops/stmt_break.o: ops/stmt_break.c ops/ops.h ops/../../util/util.h \
 ops/../data_structs.h ops/../tree.h ops/../expr.h ops/../ops/expr_addr.h \
 ops/../ops/expr_assign.h ops/../ops/expr_cast.h ops/../ops/expr_comma.h \
 ops/../ops/expr_funcall.h ops/../ops/expr_identifier.h \
 ops/../ops/expr_if.h ops/../ops/expr_op.h ops/../ops/expr_sizeof.h \
 ops/../ops/expr_val.h ops/../ops/expr_stmt.h ops/../stmt.h \
 ops/../ops/stmt_break.h ops/../ops/stmt_case.h \
 ops/../ops/stmt_case_range.h ops/../ops/stmt_code.h \
 ops/../ops/stmt_default.h ops/../ops/stmt_do.h ops/../ops/stmt_expr.h \
 ops/../ops/stmt_for.h ops/../ops/stmt_goto.h ops/../ops/stmt_if.h \
 ops/../ops/stmt_label.h ops/../ops/stmt_noop.h ops/../ops/stmt_return.h \
 ops/../ops/stmt_switch.h ops/../ops/stmt_while.h \
 ops/../ops/stmt_continue.h ops/../sym.h ops/expr_addr.h \
 ops/expr_assign.h ops/expr_cast.h ops/expr_comma.h ops/expr_funcall.h \
 ops/expr_identifier.h ops/expr_if.h ops/expr_op.h ops/expr_sizeof.h \
 ops/expr_val.h ops/stmt_break.h ops/stmt_case.h ops/stmt_case_range.h \
 ops/stmt_code.h ops/stmt_default.h ops/stmt_do.h ops/stmt_expr.h \
 ops/stmt_for.h ops/stmt_goto.h ops/stmt_if.h ops/stmt_label.h \
 ops/stmt_noop.h ops/stmt_return.h ops/stmt_switch.h ops/stmt_while.h \
 ops/../cc1.h ops/../asm_out.h ops/../asm.h ops/../fold.h ops/../const.h \
 ops/../gen_str.h ops/../gen_asm.h
ops/stmt_case.o: ops/stmt_case.c ops/ops.h ops/../../util/util.h \
 ops/../data_structs.h ops/../tree.h ops/../expr.h ops/../ops/expr_addr.h \
 ops/../ops/expr_assign.h ops/../ops/expr_cast.h ops/../ops/expr_comma.h \
 ops/../ops/expr_funcall.h ops/../ops/expr_identifier.h \
 ops/../ops/expr_if.h ops/../ops/expr_op.h ops/../ops/expr_sizeof.h \
 ops/../ops/expr_val.h ops/../ops/expr_stmt.h ops/../stmt.h \
 ops/../ops/stmt_break.h ops/../ops/stmt_case.h \
 ops/../ops/stmt_case_range.h ops/../ops/stmt_code.h \
 ops/../ops/stmt_default.h ops/../ops/stmt_do.h ops/../ops/stmt_expr.h \
 ops/../ops/stmt_for.h ops/../ops/stmt_goto.h ops/../ops/stmt_if.h \
 ops/../ops/stmt_label.h ops/../ops/stmt_noop.h ops/../ops/stmt_return.h \
 ops/../ops/stmt_switch.h ops/../ops/stmt_while.h \
 ops/../ops/stmt_continue.h ops/../sym.h ops/expr_addr.h \
 ops/expr_assign.h ops/expr_cast.h ops/expr_comma.h ops/expr_funcall.h \
 ops/expr_identifier.h ops/expr_if.h ops/expr_op.h ops/expr_sizeof.h \
 ops/expr_val.h ops/stmt_break.h ops/stmt_case.h ops/stmt_case_range.h \
 ops/stmt_code.h ops/stmt_default.h ops/stmt_do.h ops/stmt_expr.h \
 ops/stmt_for.h ops/stmt_goto.h ops/stmt_if.h ops/stmt_label.h \
 ops/stmt_noop.h ops/stmt_return.h ops/stmt_switch.h ops/stmt_while.h \
 ops/../cc1.h ops/../asm_out.h ops/../asm.h ops/../fold.h ops/../const.h \
 ops/../gen_str.h ops/../gen_asm.h
ops/stmt_case_range.o: ops/stmt_case_range.c ops/ops.h ops/../../util/util.h \
 ops/../data_structs.h ops/../tree.h ops/../expr.h ops/../ops/expr_addr.h \
 ops/../ops/expr_assign.h ops/../ops/expr_cast.h ops/../ops/expr_comma.h \
 ops/../ops/expr_funcall.h ops/../ops/expr_identifier.h \
 ops/../ops/expr_if.h ops/../ops/expr_op.h ops/../ops/expr_sizeof.h \
 ops/../ops/expr_val.h ops/../ops/expr_stmt.h ops/../stmt.h \
 ops/../ops/stmt_break.h ops/../ops/stmt_case.h \
 ops/../ops/stmt_case_range.h ops/../ops/stmt_code.h \
 ops/../ops/stmt_default.h ops/../ops/stmt_do.h ops/../ops/stmt_expr.h \
 ops/../ops/stmt_for.h ops/../ops/stmt_goto.h ops/../ops/stmt_if.h \
 ops/../ops/stmt_label.h ops/../ops/stmt_noop.h ops/../ops/stmt_return.h \
 ops/../ops/stmt_switch.h ops/../ops/stmt_while.h \
 ops/../ops/stmt_continue.h ops/../sym.h ops/expr_addr.h \
 ops/expr_assign.h ops/expr_cast.h ops/expr_comma.h ops/expr_funcall.h \
 ops/expr_identifier.h ops/expr_if.h ops/expr_op.h ops/expr_sizeof.h \
 ops/expr_val.h ops/stmt_break.h ops/stmt_case.h ops/stmt_case_range.h \
 ops/stmt_code.h ops/stmt_default.h ops/stmt_do.h ops/stmt_expr.h \
 ops/stmt_for.h ops/stmt_goto.h ops/stmt_if.h ops/stmt_label.h \
 ops/stmt_noop.h ops/stmt_return.h ops/stmt_switch.h ops/stmt_while.h \
 ops/../cc1.h ops/../asm_out.h ops/../asm.h ops/../fold.h ops/../const.h \
 ops/../gen_str.h ops/../gen_asm.h
ops/stmt_code.o: ops/stmt_code.c ops/ops.h ops/../../util/util.h \
 ops/../data_structs.h ops/../tree.h ops/../expr.h ops/../ops/expr_addr.h \
 ops/../ops/expr_assign.h ops/../ops/expr_cast.h ops/../ops/expr_comma.h \
 ops/../ops/expr_funcall.h ops/../ops/expr_identifier.h \
 ops/../ops/expr_if.h ops/../ops/expr_op.h ops/../ops/expr_sizeof.h \
 ops/../ops/expr_val.h ops/../ops/expr_stmt.h ops/../stmt.h \
 ops/../ops/stmt_break.h ops/../ops/stmt_case.h \
 ops/../ops/stmt_case_range.h ops/../ops/stmt_code.h \
 ops/../ops/stmt_default.h ops/../ops/stmt_do.h ops/../ops/stmt_expr.h \
 ops/../ops/stmt_for.h ops/../ops/stmt_goto.h ops/../ops/stmt_if.h \
 ops/../ops/stmt_label.h ops/../ops/stmt_noop.h ops/../ops/stmt_return.h \
 ops/../ops/stmt_switch.h ops/../ops/stmt_while.h \
 ops/../ops/stmt_continue.h ops/../sym.h ops/expr_addr.h \
 ops/expr_assign.h ops/expr_cast.h ops/expr_comma.h ops/expr_funcall.h \
 ops/expr_identifier.h ops/expr_if.h ops/expr_op.h ops/expr_sizeof.h \
 ops/expr_val.h ops/stmt_break.h ops/stmt_case.h ops/stmt_case_range.h \
 ops/stmt_code.h ops/stmt_default.h ops/stmt_do.h ops/stmt_expr.h \
 ops/stmt_for.h ops/stmt_goto.h ops/stmt_if.h ops/stmt_label.h \
 ops/stmt_noop.h ops/stmt_return.h ops/stmt_switch.h ops/stmt_while.h \
 ops/../cc1.h ops/../asm_out.h ops/../asm.h ops/../fold.h ops/../const.h \
 ops/../gen_str.h ops/../gen_asm.h
ops/stmt_continue.o: ops/stmt_continue.c ops/ops.h ops/../../util/util.h \
 ops/../data_structs.h ops/../tree.h ops/../expr.h ops/../ops/expr_addr.h \
 ops/../ops/expr_assign.h ops/../ops/expr_cast.h ops/../ops/expr_comma.h \
 ops/../ops/expr_funcall.h ops/../ops/expr_identifier.h \
 ops/../ops/expr_if.h ops/../ops/expr_op.h ops/../ops/expr_sizeof.h \
 ops/../ops/expr_val.h ops/../ops/expr_stmt.h ops/../stmt.h \
 ops/../ops/stmt_break.h ops/../ops/stmt_case.h \
 ops/../ops/stmt_case_range.h ops/../ops/stmt_code.h \
 ops/../ops/stmt_default.h ops/../ops/stmt_do.h ops/../ops/stmt_expr.h \
 ops/../ops/stmt_for.h ops/../ops/stmt_goto.h ops/../ops/stmt_if.h \
 ops/../ops/stmt_label.h ops/../ops/stmt_noop.h ops/../ops/stmt_return.h \
 ops/../ops/stmt_switch.h ops/../ops/stmt_while.h \
 ops/../ops/stmt_continue.h ops/../sym.h ops/expr_addr.h \
 ops/expr_assign.h ops/expr_cast.h ops/expr_comma.h ops/expr_funcall.h \
 ops/expr_identifier.h ops/expr_if.h ops/expr_op.h ops/expr_sizeof.h \
 ops/expr_val.h ops/stmt_break.h ops/stmt_case.h ops/stmt_case_range.h \
 ops/stmt_code.h ops/stmt_default.h ops/stmt_do.h ops/stmt_expr.h \
 ops/stmt_for.h ops/stmt_goto.h ops/stmt_if.h ops/stmt_label.h \
 ops/stmt_noop.h ops/stmt_return.h ops/stmt_switch.h ops/stmt_while.h \
 ops/../cc1.h ops/../asm_out.h ops/../asm.h ops/../fold.h ops/../const.h \
 ops/../gen_str.h ops/../gen_asm.h ops/stmt_continue.h
ops/stmt_default.o: ops/stmt_default.c ops/ops.h ops/../../util/util.h \
 ops/../data_structs.h ops/../tree.h ops/../expr.h ops/../ops/expr_addr.h \
 ops/../ops/expr_assign.h ops/../ops/expr_cast.h ops/../ops/expr_comma.h \
 ops/../ops/expr_funcall.h ops/../ops/expr_identifier.h \
 ops/../ops/expr_if.h ops/../ops/expr_op.h ops/../ops/expr_sizeof.h \
 ops/../ops/expr_val.h ops/../ops/expr_stmt.h ops/../stmt.h \
 ops/../ops/stmt_break.h ops/../ops/stmt_case.h \
 ops/../ops/stmt_case_range.h ops/../ops/stmt_code.h \
 ops/../ops/stmt_default.h ops/../ops/stmt_do.h ops/../ops/stmt_expr.h \
 ops/../ops/stmt_for.h ops/../ops/stmt_goto.h ops/../ops/stmt_if.h \
 ops/../ops/stmt_label.h ops/../ops/stmt_noop.h ops/../ops/stmt_return.h \
 ops/../ops/stmt_switch.h ops/../ops/stmt_while.h \
 ops/../ops/stmt_continue.h ops/../sym.h ops/expr_addr.h \
 ops/expr_assign.h ops/expr_cast.h ops/expr_comma.h ops/expr_funcall.h \
 ops/expr_identifier.h ops/expr_if.h ops/expr_op.h ops/expr_sizeof.h \
 ops/expr_val.h ops/stmt_break.h ops/stmt_case.h ops/stmt_case_range.h \
 ops/stmt_code.h ops/stmt_default.h ops/stmt_do.h ops/stmt_expr.h \
 ops/stmt_for.h ops/stmt_goto.h ops/stmt_if.h ops/stmt_label.h \
 ops/stmt_noop.h ops/stmt_return.h ops/stmt_switch.h ops/stmt_while.h \
 ops/../cc1.h ops/../asm_out.h ops/../asm.h ops/../fold.h ops/../const.h \
 ops/../gen_str.h ops/../gen_asm.h
ops/stmt_do.o: ops/stmt_do.c ops/ops.h ops/../../util/util.h \
 ops/../data_structs.h ops/../tree.h ops/../expr.h ops/../ops/expr_addr.h \
 ops/../ops/expr_assign.h ops/../ops/expr_cast.h ops/../ops/expr_comma.h \
 ops/../ops/expr_funcall.h ops/../ops/expr_identifier.h \
 ops/../ops/expr_if.h ops/../ops/expr_op.h ops/../ops/expr_sizeof.h \
 ops/../ops/expr_val.h ops/../ops/expr_stmt.h ops/../stmt.h \
 ops/../ops/stmt_break.h ops/../ops/stmt_case.h \
 ops/../ops/stmt_case_range.h ops/../ops/stmt_code.h \
 ops/../ops/stmt_default.h ops/../ops/stmt_do.h ops/../ops/stmt_expr.h \
 ops/../ops/stmt_for.h ops/../ops/stmt_goto.h ops/../ops/stmt_if.h \
 ops/../ops/stmt_label.h ops/../ops/stmt_noop.h ops/../ops/stmt_return.h \
 ops/../ops/stmt_switch.h ops/../ops/stmt_while.h \
 ops/../ops/stmt_continue.h ops/../sym.h ops/expr_addr.h \
 ops/expr_assign.h ops/expr_cast.h ops/expr_comma.h ops/expr_funcall.h \
 ops/expr_identifier.h ops/expr_if.h ops/expr_op.h ops/expr_sizeof.h \
 ops/expr_val.h ops/stmt_break.h ops/stmt_case.h ops/stmt_case_range.h \
 ops/stmt_code.h ops/stmt_default.h ops/stmt_do.h ops/stmt_expr.h \
 ops/stmt_for.h ops/stmt_goto.h ops/stmt_if.h ops/stmt_label.h \
 ops/stmt_noop.h ops/stmt_return.h ops/stmt_switch.h ops/stmt_while.h \
 ops/../cc1.h ops/../asm_out.h ops/../asm.h ops/../fold.h ops/../const.h \
 ops/../gen_str.h ops/../gen_asm.h
ops/stmt_expr.o: ops/stmt_expr.c ops/ops.h ops/../../util/util.h \
 ops/../data_structs.h ops/../tree.h ops/../expr.h ops/../ops/expr_addr.h \
 ops/../ops/expr_assign.h ops/../ops/expr_cast.h ops/../ops/expr_comma.h \
 ops/../ops/expr_funcall.h ops/../ops/expr_identifier.h \
 ops/../ops/expr_if.h ops/../ops/expr_op.h ops/../ops/expr_sizeof.h \
 ops/../ops/expr_val.h ops/../ops/expr_stmt.h ops/../stmt.h \
 ops/../ops/stmt_break.h ops/../ops/stmt_case.h \
 ops/../ops/stmt_case_range.h ops/../ops/stmt_code.h \
 ops/../ops/stmt_default.h ops/../ops/stmt_do.h ops/../ops/stmt_expr.h \
 ops/../ops/stmt_for.h ops/../ops/stmt_goto.h ops/../ops/stmt_if.h \
 ops/../ops/stmt_label.h ops/../ops/stmt_noop.h ops/../ops/stmt_return.h \
 ops/../ops/stmt_switch.h ops/../ops/stmt_while.h \
 ops/../ops/stmt_continue.h ops/../sym.h ops/expr_addr.h \
 ops/expr_assign.h ops/expr_cast.h ops/expr_comma.h ops/expr_funcall.h \
 ops/expr_identifier.h ops/expr_if.h ops/expr_op.h ops/expr_sizeof.h \
 ops/expr_val.h ops/stmt_break.h ops/stmt_case.h ops/stmt_case_range.h \
 ops/stmt_code.h ops/stmt_default.h ops/stmt_do.h ops/stmt_expr.h \
 ops/stmt_for.h ops/stmt_goto.h ops/stmt_if.h ops/stmt_label.h \
 ops/stmt_noop.h ops/stmt_return.h ops/stmt_switch.h ops/stmt_while.h \
 ops/../cc1.h ops/../asm_out.h ops/../asm.h ops/../fold.h ops/../const.h \
 ops/../gen_str.h ops/../gen_asm.h
ops/stmt_for.o: ops/stmt_for.c ops/ops.h ops/../../util/util.h \
 ops/../data_structs.h ops/../tree.h ops/../expr.h ops/../ops/expr_addr.h \
 ops/../ops/expr_assign.h ops/../ops/expr_cast.h ops/../ops/expr_comma.h \
 ops/../ops/expr_funcall.h ops/../ops/expr_identifier.h \
 ops/../ops/expr_if.h ops/../ops/expr_op.h ops/../ops/expr_sizeof.h \
 ops/../ops/expr_val.h ops/../ops/expr_stmt.h ops/../stmt.h \
 ops/../ops/stmt_break.h ops/../ops/stmt_case.h \
 ops/../ops/stmt_case_range.h ops/../ops/stmt_code.h \
 ops/../ops/stmt_default.h ops/../ops/stmt_do.h ops/../ops/stmt_expr.h \
 ops/../ops/stmt_for.h ops/../ops/stmt_goto.h ops/../ops/stmt_if.h \
 ops/../ops/stmt_label.h ops/../ops/stmt_noop.h ops/../ops/stmt_return.h \
 ops/../ops/stmt_switch.h ops/../ops/stmt_while.h \
 ops/../ops/stmt_continue.h ops/../sym.h ops/expr_addr.h \
 ops/expr_assign.h ops/expr_cast.h ops/expr_comma.h ops/expr_funcall.h \
 ops/expr_identifier.h ops/expr_if.h ops/expr_op.h ops/expr_sizeof.h \
 ops/expr_val.h ops/stmt_break.h ops/stmt_case.h ops/stmt_case_range.h \
 ops/stmt_code.h ops/stmt_default.h ops/stmt_do.h ops/stmt_expr.h \
 ops/stmt_for.h ops/stmt_goto.h ops/stmt_if.h ops/stmt_label.h \
 ops/stmt_noop.h ops/stmt_return.h ops/stmt_switch.h ops/stmt_while.h \
 ops/../cc1.h ops/../asm_out.h ops/../asm.h ops/../fold.h ops/../const.h \
 ops/../gen_str.h ops/../gen_asm.h
ops/stmt_goto.o: ops/stmt_goto.c ops/ops.h ops/../../util/util.h \
 ops/../data_structs.h ops/../tree.h ops/../expr.h ops/../ops/expr_addr.h \
 ops/../ops/expr_assign.h ops/../ops/expr_cast.h ops/../ops/expr_comma.h \
 ops/../ops/expr_funcall.h ops/../ops/expr_identifier.h \
 ops/../ops/expr_if.h ops/../ops/expr_op.h ops/../ops/expr_sizeof.h \
 ops/../ops/expr_val.h ops/../ops/expr_stmt.h ops/../stmt.h \
 ops/../ops/stmt_break.h ops/../ops/stmt_case.h \
 ops/../ops/stmt_case_range.h ops/../ops/stmt_code.h \
 ops/../ops/stmt_default.h ops/../ops/stmt_do.h ops/../ops/stmt_expr.h \
 ops/../ops/stmt_for.h ops/../ops/stmt_goto.h ops/../ops/stmt_if.h \
 ops/../ops/stmt_label.h ops/../ops/stmt_noop.h ops/../ops/stmt_return.h \
 ops/../ops/stmt_switch.h ops/../ops/stmt_while.h \
 ops/../ops/stmt_continue.h ops/../sym.h ops/expr_addr.h \
 ops/expr_assign.h ops/expr_cast.h ops/expr_comma.h ops/expr_funcall.h \
 ops/expr_identifier.h ops/expr_if.h ops/expr_op.h ops/expr_sizeof.h \
 ops/expr_val.h ops/stmt_break.h ops/stmt_case.h ops/stmt_case_range.h \
 ops/stmt_code.h ops/stmt_default.h ops/stmt_do.h ops/stmt_expr.h \
 ops/stmt_for.h ops/stmt_goto.h ops/stmt_if.h ops/stmt_label.h \
 ops/stmt_noop.h ops/stmt_return.h ops/stmt_switch.h ops/stmt_while.h \
 ops/../cc1.h ops/../asm_out.h ops/../asm.h ops/../fold.h ops/../const.h \
 ops/../gen_str.h ops/../gen_asm.h
ops/stmt_if.o: ops/stmt_if.c ops/ops.h ops/../../util/util.h \
 ops/../data_structs.h ops/../tree.h ops/../expr.h ops/../ops/expr_addr.h \
 ops/../ops/expr_assign.h ops/../ops/expr_cast.h ops/../ops/expr_comma.h \
 ops/../ops/expr_funcall.h ops/../ops/expr_identifier.h \
 ops/../ops/expr_if.h ops/../ops/expr_op.h ops/../ops/expr_sizeof.h \
 ops/../ops/expr_val.h ops/../ops/expr_stmt.h ops/../stmt.h \
 ops/../ops/stmt_break.h ops/../ops/stmt_case.h \
 ops/../ops/stmt_case_range.h ops/../ops/stmt_code.h \
 ops/../ops/stmt_default.h ops/../ops/stmt_do.h ops/../ops/stmt_expr.h \
 ops/../ops/stmt_for.h ops/../ops/stmt_goto.h ops/../ops/stmt_if.h \
 ops/../ops/stmt_label.h ops/../ops/stmt_noop.h ops/../ops/stmt_return.h \
 ops/../ops/stmt_switch.h ops/../ops/stmt_while.h \
 ops/../ops/stmt_continue.h ops/../sym.h ops/expr_addr.h \
 ops/expr_assign.h ops/expr_cast.h ops/expr_comma.h ops/expr_funcall.h \
 ops/expr_identifier.h ops/expr_if.h ops/expr_op.h ops/expr_sizeof.h \
 ops/expr_val.h ops/stmt_break.h ops/stmt_case.h ops/stmt_case_range.h \
 ops/stmt_code.h ops/stmt_default.h ops/stmt_do.h ops/stmt_expr.h \
 ops/stmt_for.h ops/stmt_goto.h ops/stmt_if.h ops/stmt_label.h \
 ops/stmt_noop.h ops/stmt_return.h ops/stmt_switch.h ops/stmt_while.h \
 ops/../cc1.h ops/../asm_out.h ops/../asm.h ops/../fold.h ops/../const.h \
 ops/../gen_str.h ops/../gen_asm.h
ops/stmt_label.o: ops/stmt_label.c ops/ops.h ops/../../util/util.h \
 ops/../data_structs.h ops/../tree.h ops/../expr.h ops/../ops/expr_addr.h \
 ops/../ops/expr_assign.h ops/../ops/expr_cast.h ops/../ops/expr_comma.h \
 ops/../ops/expr_funcall.h ops/../ops/expr_identifier.h \
 ops/../ops/expr_if.h ops/../ops/expr_op.h ops/../ops/expr_sizeof.h \
 ops/../ops/expr_val.h ops/../ops/expr_stmt.h ops/../stmt.h \
 ops/../ops/stmt_break.h ops/../ops/stmt_case.h \
 ops/../ops/stmt_case_range.h ops/../ops/stmt_code.h \
 ops/../ops/stmt_default.h ops/../ops/stmt_do.h ops/../ops/stmt_expr.h \
 ops/../ops/stmt_for.h ops/../ops/stmt_goto.h ops/../ops/stmt_if.h \
 ops/../ops/stmt_label.h ops/../ops/stmt_noop.h ops/../ops/stmt_return.h \
 ops/../ops/stmt_switch.h ops/../ops/stmt_while.h \
 ops/../ops/stmt_continue.h ops/../sym.h ops/expr_addr.h \
 ops/expr_assign.h ops/expr_cast.h ops/expr_comma.h ops/expr_funcall.h \
 ops/expr_identifier.h ops/expr_if.h ops/expr_op.h ops/expr_sizeof.h \
 ops/expr_val.h ops/stmt_break.h ops/stmt_case.h ops/stmt_case_range.h \
 ops/stmt_code.h ops/stmt_default.h ops/stmt_do.h ops/stmt_expr.h \
 ops/stmt_for.h ops/stmt_goto.h ops/stmt_if.h ops/stmt_label.h \
 ops/stmt_noop.h ops/stmt_return.h ops/stmt_switch.h ops/stmt_while.h \
 ops/../cc1.h ops/../asm_out.h ops/../asm.h ops/../fold.h ops/../const.h \
 ops/../gen_str.h ops/../gen_asm.h
ops/stmt_noop.o: ops/stmt_noop.c ops/ops.h ops/../../util/util.h \
 ops/../data_structs.h ops/../tree.h ops/../expr.h ops/../ops/expr_addr.h \
 ops/../ops/expr_assign.h ops/../ops/expr_cast.h ops/../ops/expr_comma.h \
 ops/../ops/expr_funcall.h ops/../ops/expr_identifier.h \
 ops/../ops/expr_if.h ops/../ops/expr_op.h ops/../ops/expr_sizeof.h \
 ops/../ops/expr_val.h ops/../ops/expr_stmt.h ops/../stmt.h \
 ops/../ops/stmt_break.h ops/../ops/stmt_case.h \
 ops/../ops/stmt_case_range.h ops/../ops/stmt_code.h \
 ops/../ops/stmt_default.h ops/../ops/stmt_do.h ops/../ops/stmt_expr.h \
 ops/../ops/stmt_for.h ops/../ops/stmt_goto.h ops/../ops/stmt_if.h \
 ops/../ops/stmt_label.h ops/../ops/stmt_noop.h ops/../ops/stmt_return.h \
 ops/../ops/stmt_switch.h ops/../ops/stmt_while.h \
 ops/../ops/stmt_continue.h ops/../sym.h ops/expr_addr.h \
 ops/expr_assign.h ops/expr_cast.h ops/expr_comma.h ops/expr_funcall.h \
 ops/expr_identifier.h ops/expr_if.h ops/expr_op.h ops/expr_sizeof.h \
 ops/expr_val.h ops/stmt_break.h ops/stmt_case.h ops/stmt_case_range.h \
 ops/stmt_code.h ops/stmt_default.h ops/stmt_do.h ops/stmt_expr.h \
 ops/stmt_for.h ops/stmt_goto.h ops/stmt_if.h ops/stmt_label.h \
 ops/stmt_noop.h ops/stmt_return.h ops/stmt_switch.h ops/stmt_while.h \
 ops/../cc1.h ops/../asm_out.h ops/../asm.h ops/../fold.h ops/../const.h \
 ops/../gen_str.h ops/../gen_asm.h
ops/stmt_return.o: ops/stmt_return.c ops/ops.h ops/../../util/util.h \
 ops/../data_structs.h ops/../tree.h ops/../expr.h ops/../ops/expr_addr.h \
 ops/../ops/expr_assign.h ops/../ops/expr_cast.h ops/../ops/expr_comma.h \
 ops/../ops/expr_funcall.h ops/../ops/expr_identifier.h \
 ops/../ops/expr_if.h ops/../ops/expr_op.h ops/../ops/expr_sizeof.h \
 ops/../ops/expr_val.h ops/../ops/expr_stmt.h ops/../stmt.h \
 ops/../ops/stmt_break.h ops/../ops/stmt_case.h \
 ops/../ops/stmt_case_range.h ops/../ops/stmt_code.h \
 ops/../ops/stmt_default.h ops/../ops/stmt_do.h ops/../ops/stmt_expr.h \
 ops/../ops/stmt_for.h ops/../ops/stmt_goto.h ops/../ops/stmt_if.h \
 ops/../ops/stmt_label.h ops/../ops/stmt_noop.h ops/../ops/stmt_return.h \
 ops/../ops/stmt_switch.h ops/../ops/stmt_while.h \
 ops/../ops/stmt_continue.h ops/../sym.h ops/expr_addr.h \
 ops/expr_assign.h ops/expr_cast.h ops/expr_comma.h ops/expr_funcall.h \
 ops/expr_identifier.h ops/expr_if.h ops/expr_op.h ops/expr_sizeof.h \
 ops/expr_val.h ops/stmt_break.h ops/stmt_case.h ops/stmt_case_range.h \
 ops/stmt_code.h ops/stmt_default.h ops/stmt_do.h ops/stmt_expr.h \
 ops/stmt_for.h ops/stmt_goto.h ops/stmt_if.h ops/stmt_label.h \
 ops/stmt_noop.h ops/stmt_return.h ops/stmt_switch.h ops/stmt_while.h \
 ops/../cc1.h ops/../asm_out.h ops/../asm.h ops/../fold.h ops/../const.h \
 ops/../gen_str.h ops/../gen_asm.h
ops/stmt_switch.o: ops/stmt_switch.c ops/ops.h ops/../../util/util.h \
 ops/../data_structs.h ops/../tree.h ops/../expr.h ops/../ops/expr_addr.h \
 ops/../ops/expr_assign.h ops/../ops/expr_cast.h ops/../ops/expr_comma.h \
 ops/../ops/expr_funcall.h ops/../ops/expr_identifier.h \
 ops/../ops/expr_if.h ops/../ops/expr_op.h ops/../ops/expr_sizeof.h \
 ops/../ops/expr_val.h ops/../ops/expr_stmt.h ops/../stmt.h \
 ops/../ops/stmt_break.h ops/../ops/stmt_case.h \
 ops/../ops/stmt_case_range.h ops/../ops/stmt_code.h \
 ops/../ops/stmt_default.h ops/../ops/stmt_do.h ops/../ops/stmt_expr.h \
 ops/../ops/stmt_for.h ops/../ops/stmt_goto.h ops/../ops/stmt_if.h \
 ops/../ops/stmt_label.h ops/../ops/stmt_noop.h ops/../ops/stmt_return.h \
 ops/../ops/stmt_switch.h ops/../ops/stmt_while.h \
 ops/../ops/stmt_continue.h ops/../sym.h ops/expr_addr.h \
 ops/expr_assign.h ops/expr_cast.h ops/expr_comma.h ops/expr_funcall.h \
 ops/expr_identifier.h ops/expr_if.h ops/expr_op.h ops/expr_sizeof.h \
 ops/expr_val.h ops/stmt_break.h ops/stmt_case.h ops/stmt_case_range.h \
 ops/stmt_code.h ops/stmt_default.h ops/stmt_do.h ops/stmt_expr.h \
 ops/stmt_for.h ops/stmt_goto.h ops/stmt_if.h ops/stmt_label.h \
 ops/stmt_noop.h ops/stmt_return.h ops/stmt_switch.h ops/stmt_while.h \
 ops/../cc1.h ops/../asm_out.h ops/../asm.h ops/../fold.h ops/../const.h \
 ops/../gen_str.h ops/../gen_asm.h ops/../enum.h ops/../../util/alloc.h
ops/stmt_while.o: ops/stmt_while.c ops/ops.h ops/../../util/util.h \
 ops/../data_structs.h ops/../tree.h ops/../expr.h ops/../ops/expr_addr.h \
 ops/../ops/expr_assign.h ops/../ops/expr_cast.h ops/../ops/expr_comma.h \
 ops/../ops/expr_funcall.h ops/../ops/expr_identifier.h \
 ops/../ops/expr_if.h ops/../ops/expr_op.h ops/../ops/expr_sizeof.h \
 ops/../ops/expr_val.h ops/../ops/expr_stmt.h ops/../stmt.h \
 ops/../ops/stmt_break.h ops/../ops/stmt_case.h \
 ops/../ops/stmt_case_range.h ops/../ops/stmt_code.h \
 ops/../ops/stmt_default.h ops/../ops/stmt_do.h ops/../ops/stmt_expr.h \
 ops/../ops/stmt_for.h ops/../ops/stmt_goto.h ops/../ops/stmt_if.h \
 ops/../ops/stmt_label.h ops/../ops/stmt_noop.h ops/../ops/stmt_return.h \
 ops/../ops/stmt_switch.h ops/../ops/stmt_while.h \
 ops/../ops/stmt_continue.h ops/../sym.h ops/expr_addr.h \
 ops/expr_assign.h ops/expr_cast.h ops/expr_comma.h ops/expr_funcall.h \
 ops/expr_identifier.h ops/expr_if.h ops/expr_op.h ops/expr_sizeof.h \
 ops/expr_val.h ops/stmt_break.h ops/stmt_case.h ops/stmt_case_range.h \
 ops/stmt_code.h ops/stmt_default.h ops/stmt_do.h ops/stmt_expr.h \
 ops/stmt_for.h ops/stmt_goto.h ops/stmt_if.h ops/stmt_label.h \
 ops/stmt_noop.h ops/stmt_return.h ops/stmt_switch.h ops/stmt_while.h \
 ops/../cc1.h ops/../asm_out.h ops/../asm.h ops/../fold.h ops/../const.h \
 ops/../gen_str.h ops/../gen_asm.h
