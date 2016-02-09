#ifndef EXPR_ASSIGN_H
#define EXPR_ASSIGN_H

EXPR_DEFS(assign);

int expr_must_lvalue(expr *e, const char *desc);
void bitfield_trunc_check(decl *mem, expr *from);
void expr_assign_const_check(expr *e, where *w);

struct irval *gen_ir_assign_bitfield(
		struct irval *lhs,
		struct irval *rhs,
		expr *estruct,
		struct irctx *ctx);

#endif
