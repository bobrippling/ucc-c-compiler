func_fold    fold_expr_op;
func_gen     gen_expr_op;
func_str     str_expr_op;
func_gen     gen_expr_str_op;
func_mutate_expr mutate_expr_op;
func_gen     gen_expr_style_op;

#if defined(__GNUC__) && (__GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4))
#  define ucc_wur __attribute__((warn_unused_result))
#else
#  define ucc_wur
#endif

type *op_required_promotion(
		enum op_type op,
		expr *lhs, expr *rhs,
		where *w, const char *desc /* maybe null */,
		type **plhs, type **prhs)
	ucc_wur;

type *op_promote_types(
		enum op_type op,
		expr **plhs, expr **prhs,
		symtable *stab,
		where *w, const char *desc)
	ucc_wur;

void expr_promote_default(expr **pe, symtable *stab);
void expr_promote_int_if_smaller(expr **pe, symtable *stab);

/* called from op code and deref code
 * op code checks for 0 to len-1,
 * deref code checks for *len.
 * This way there's no duplicate warnings
 *
 * returns 1 if a warning was printed, 0 otherwise
 */
int fold_check_bounds(expr *e, int chk_one_past_end);

void expr_check_sign(const char *desc,
		expr *lhs, expr *rhs, where *w);

void gen_op_trapv(type *evaltt, const out_val **eval, out_ctx *octx);
