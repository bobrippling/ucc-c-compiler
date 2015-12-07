#ifndef GEN_IR_H
#define GEN_IR_H

#define IRTODO(...) /*ICW(__VA_ARGS__)*/

void gen_ir(symtable_global *);

typedef struct irctx irctx;
typedef struct irval irval;

irval *gen_ir_expr(const struct expr *, irctx *);
void gen_ir_stmt(const struct stmt *, irctx *);

void gen_ir_comment(irctx *, const char *, ...)
	ucc_printflike(2, 3);

const char *irtype_str(type *);
const char *irval_str(irval *);
const char *ir_op_str(enum op_type, int arith_rshift);

typedef unsigned irid;

irval *irval_from_id(irid);
irval *irval_from_l(type *, integral_t);
struct sym;
irval *irval_from_sym(irctx *, struct sym *);
irval *irval_from_lbl(irctx *, char *);
irval *irval_from_noop(void);

void irval_free(irval *);
void irval_free_abi(void *);

#endif
