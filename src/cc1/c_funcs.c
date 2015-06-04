#include <stddef.h>

#include "expr.h"
#include "c_funcs.h"

#include "type_is.h"
#include "cc1.h"

void c_func_check_free(expr *arg)
{
	int warn = 0;

	arg = expr_skip_casts(arg);

	if(expr_kind(arg, identifier)){
		sym *sym = arg->bits.ident.bits.ident.sym;

		if(sym && type_is_array(sym->decl->ref))
			warn = 1;

	}else if(expr_kind(arg, addr)){
		expr *addrof = expr_addr_target(arg);

		if(expr_kind(addrof, identifier))
			warn = 1;
	}

	if(warn){
		cc1_warn_at(&arg->where, free_nonheap, "free() of non-heap object");
	}
}

static type *find_sizeof(expr *e)
{
	if(expr_kind(e, sizeof))
		return expr_sizeof_type(e);

	if(expr_kind(e, op) && e->bits.op.op == op_multiply){
		/* detect 2 * sizeof(x) */
		type *t = find_sizeof(e->lhs);
		if(t)
			return t;
		return find_sizeof(e->rhs);
	}

	return NULL;
}

static int warn_if_type_mismatch(type *a, type *b, where *loc, const char *fn)
{
	char buf[TYPE_STATIC_BUFSIZ];

	/* if different, warn, unless one is void */
	if(type_cmp(a, b, 0) & (TYPE_EQUAL_ANY | TYPE_QUAL_ADD | TYPE_QUAL_SUB))
		return 0;

	if(type_is_void(a) || type_is_void(b))
		return 0;

	cc1_warn_at(loc, sizeof_pointer_memaccess,
			"%s with different types '%s' and '%s'",
			fn,
			type_to_str_r(buf, a),
			type_to_str(b));

	return 1;
}

void c_func_check_mem(expr *ptr_args[], expr *sizeof_arg, const char *func)
{
	type *const sztype = find_sizeof(sizeof_arg);
	expr **i;

	if(!sztype)
		return;

	for(i = ptr_args; *i; i++){
		expr *e = expr_skip_casts(*i);
		type *ptr_ty = type_is_ptr(e->tree_type);

		if(!ptr_ty)
			continue;

		if(warn_if_type_mismatch(ptr_ty, sztype, &e->where, func))
			break;
	}
}
