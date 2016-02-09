#include <stddef.h>
#include <stdio.h>

#include "../util/dynarray.h"

#include "expr.h"
#include "c_funcs.h"

#include "type_nav.h"
#include "type_is.h"
#include "cc1.h"

#include "ops/expr_identifier.h"
#include "ops/expr_addr.h"
#include "ops/expr_sizeof.h"
#include "ops/expr_op.h"

void c_func_check_free(expr *arg)
{
	int warn = 0;

	arg = expr_skip_all_casts(arg);

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

static int warn_if_type_mismatch(
		type *a, type *b,
		where *loc, const char *fn,
		const unsigned char *warnp)
{
	char buf[TYPE_STATIC_BUFSIZ];

	/* if different, warn, unless one is void */
	if(type_cmp(a, b, 0) & (TYPE_EQUAL_ANY | TYPE_QUAL_ADD | TYPE_QUAL_SUB))
		return 0;

	if(type_is_void(a) || type_is_void(b))
		return 0;

	cc1_warn_at_w(loc, warnp,
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
		expr *e = expr_skip_all_casts(*i);
		type *ptr_ty = type_is_ptr(e->tree_type);

		if(!ptr_ty)
			continue;

		if(warn_if_type_mismatch(
					ptr_ty, sztype,
					&e->where, func,
					&cc1_warning.sizeof_pointer_memaccess))
		{
			break;
		}
	}
}

void c_func_check_malloc(expr *malloc_call_expr, type *assigned_to)
{
	type *sizeof_type;

	if(dynarray_count(malloc_call_expr->funcargs) != 1)
		return;

	sizeof_type = find_sizeof(malloc_call_expr->funcargs[0]);
	if(!sizeof_type)
		return;

	warn_if_type_mismatch(
			assigned_to, type_ptr_to(sizeof_type),
			&malloc_call_expr->where,
			"malloc assignment",
			&cc1_warning.malloc_type_mismatch);
}
