func_fold    fold_expr_op;
func_gen     gen_expr_op;
func_const   const_expr_op;
func_str     str_expr_op;
func_gen     gen_expr_str_op;
func_mutate_expr mutate_expr_op;
func_gen     gen_expr_style_op;

#define op_deref_expr(e) ((e)->lhs)

#if defined(__GNUC__) && (__GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4))
#  define ucc_wur __attribute__((warn_unused_result))
#else
#  define ucc_wur
#endif

decl *op_promote_types(enum op_type op, expr *lhs, expr *rhs, where *w) ucc_wur;
