#include <stdio.h>
#include <stdarg.h>

#include "../../util/util.h"
#include "../../util/dynarray.h"
#include "../../util/alloc.h"

#include "../data_structs.h"
#include "__builtin.h"

/* TODO: constant_p, types_compatible_p */

static decl *
builtin_unreachable()
{
	decl *d = decl_new_void();
	funcargs *fargs;

	d->desc = decl_desc_func_new(d, NULL);
	fargs = funcargs_new();
	fargs->args_void = 1; /* x(void) */

	d->desc->bits.func = fargs;

	d->spel = ustrdup("__builtin_unreachable");

	decl_attr_append(&d->attr, decl_attr_new(attr_noreturn));

	return d;
}

decl **builtin_funcs(void)
{
	decl **funcs = NULL, **i;

	dynarray_add((void ***)&funcs, builtin_unreachable());

	for(i = funcs; i && *i; i++)
		(*i)->builtin = 1;

	return funcs;
}

void builtin_fold(expr *e)
{
	/* funcall */
	(void)e;
}

void builtin_gen(expr *e)
{
	(void)e;
}
