#ifndef GEN_IR_H
#define GEN_IR_H

void gen_ir(symtable_global *);

typedef struct irctx irctx;
typedef struct irval irval;

irval *gen_ir_expr(irctx *, const struct expr *);
void gen_ir_stmt(irctx *, const struct stmt *);

const char *irtype_str(type *);

typedef unsigned irid;

irval *irval_from_id(irid);
irval *irval_from_l(integral_t);
struct sym;
irval *irval_from_sym(struct sym *);
irval *irval_from_lbl(char *);

#endif
