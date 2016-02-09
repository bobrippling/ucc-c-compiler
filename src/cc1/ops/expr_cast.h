EXPR_DEFS(cast);

#define expr_cast_child(e) ((e)->expr)
#define expr_cast_is_lval2rval(e) (!(e)->bits.cast_to)
#define expr_cast_is_implicit(e) ((e)->expr_cast_implicit)

void fold_expr_cast_descend(
		expr *e, symtable *stab, int descend);

unsigned gen_ir_lval2rval_bitfield(
		unsigned unmasked,
		const expr *child,
		struct irctx *ctx);
