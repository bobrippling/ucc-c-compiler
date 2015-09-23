#ifndef GEN_IR_H
#define GEN_IR_H

void gen_ir(symtable_global *);

typedef struct irctx irctx;
typedef struct irval irval;

irval *gen_ir_expr(const struct expr *, irctx *);
void gen_ir_stmt(const struct stmt *, irctx *);

const char *irtype_str(type *);
const char *irval_str(irval *);

typedef unsigned irid;

irval *irval_from_id(irid);
irval *irval_from_l(type *, integral_t);
struct sym;
irval *irval_from_sym(irctx *, struct sym *);
irval *irval_from_lbl(irctx *, char *);

void irval_free(irval *);
void irval_free_abi(void *);

#endif
