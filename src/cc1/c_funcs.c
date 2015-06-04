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
