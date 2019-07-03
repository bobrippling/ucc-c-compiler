#ifndef C_FUNCS_H
#define C_FUNCS_H

void c_func_check_free(expr *);
void c_func_check_mem(expr *ptr_args[], expr *sizeof_arg, const char *func);
void c_func_check_malloc(expr *malloc_call_expr, type *assigned_to);

#endif
