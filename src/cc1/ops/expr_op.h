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

type_ref *op_required_promotion(enum op_type op, expr *lhs, expr *rhs, where *w, type_ref **plhs, type_ref **prhs) ucc_wur;
type_ref *op_promote_types(enum op_type op, expr **plhs, expr **prhs, where *w, symtable *stab) ucc_wur;
void expr_promote_default(expr **pe, symtable *stab);

/* called from op code and deref code
 * op code checks for 0 to len-1,
 * deref code checks for *len.
 * This way there's no duplicate warnings
 *
 * returns 1 if a warning was printed, 0 otherwise
 */
int fold_check_bounds(expr *e, int chk_one_past_end);
