EXPR_DEFS(funcall);

int expr_func_passable(expr *e);
attribute *func_or_builtin_attr_present(expr *, enum attribute_type);

expr *expr_new_funcall(void);
