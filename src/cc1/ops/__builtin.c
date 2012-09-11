#include <stdio.h>
#include <stdarg.h>

#include "../../util/util.h"
#include "../../util/dynarray.h"
#include "../../util/alloc.h"

#include "../data_structs.h"
#include "__builtin.h"

/* TODO: constant_p, types_compatible_p */

static decl *
builtin_new_decl(const char *sp)
{
	decl *d = decl_new_void();
	funcargs *fargs;

	d->type->primitive = type_void;

	d->desc = decl_desc_func_new(d, NULL);
	fargs = funcargs_new();
	fargs->args_void = 1; /* x(void) */

	d->desc->bits.func = fargs;

	d->spel = ustrprintf("__builtin_%s", sp);

	return d;
}

static decl *
builtin_new_decl_bool(const char *sp)
{
	decl *d = builtin_new_decl(sp);
	d->type->primitive = type_int;
	return d;
}

static decl *
builtin_unreachable()
{
	decl *d = builtin_new_decl("unreachable");
	decl_attr_append(&d->attr, decl_attr_new(attr_noreturn));
	return d;
}

static decl *
builtin_types_compatible_p()
{
	decl *d = builtin_new_decl_bool("types_compatible_p");
	return d;
}

static decl *
builtin_constant_p()
{
	decl *d = builtin_new_decl_bool("constant_p");
	return d;
}

decl **builtin_funcs(void)
{
	decl **funcs = NULL, **i;

	/* TODO: struct/table */
	dynarray_add((void ***)&funcs, builtin_unreachable());
	dynarray_add((void ***)&funcs, builtin_types_compatible_p());
	dynarray_add((void ***)&funcs, builtin_constant_p());

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
