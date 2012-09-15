#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "../../util/util.h"
#include "../../util/dynarray.h"
#include "../../util/alloc.h"

#include "../data_structs.h"
#include "__builtin.h"

#include "../cc1.h"
#include "../tokenise.h"
#include "../parse.h"
#include "../fold.h"

#include "../const.h"

#include "../out/out.h"

#define PREFIX "__builtin_"

typedef expr *func_builtin_parse(void);

static func_builtin_parse parse_unreachable,
													parse_compatible_p,
													parse_constant_p,
													parse_frame_address;

typedef struct
{
	const char *sp;
	func_builtin_parse *parser;
} builtin_table;

builtin_table builtins[] = {
	{ "unreachable", parse_unreachable },
	{ "trap", parse_unreachable }, /* same */

	{ "types_compatible_p", parse_compatible_p },
	{ "constant_p", parse_constant_p },

	{ "frame_address", parse_frame_address },

	{ NULL, NULL }
};

static builtin_table *builtin_find(const char *sp)
{
	const int prefix_len = strlen(PREFIX);

	if(!strncmp(sp, PREFIX, prefix_len)){
		int i;
		sp += prefix_len;

		for(i = 0; builtins[i].sp; i++)
			if(!strcmp(sp, builtins[i].sp))
				return &builtins[i];
	}

	return NULL;
}

expr *builtin_parse(const char *sp)
{
	builtin_table *b = builtin_find(sp);

	if(b){
		expr *(*f)(void) = b->parser;

		if(f)
			return f();
	}

	return NULL;
}

#define expr_mutate_builtin(exp, to)  \
	exp->f_fold = fold_ ## to

#define expr_mutate_builtin_no_gen(exp, to) \
	expr_mutate_builtin(exp, to),             \
	exp->f_gen        = NULL,                 \
	exp->f_const_fold = const_ ## to

static void wur_builtin(expr *e)
{
	e->freestanding = 0; /* needs use */
}

static void builtin_gen_undefined(expr *e, symtable *stab)
{
	(void)e;
	(void)stab;
	out_undefined();
	out_push_i(NULL, 0);
}

static expr *parse_any_args(void)
{
	expr *fcall = expr_new_funcall();
	fcall->funcargs = parse_funcargs();
	return fcall;
}

/* --- unreachable */

static void fold_unreachable(expr *e, symtable *stab)
{
	(void)stab;

	e->tree_type = decl_new_void();
	decl_attr_append(&e->tree_type->attr, decl_attr_new(attr_noreturn));

	wur_builtin(e);
}

static expr *parse_unreachable(void)
{
	expr *fcall = expr_new_funcall();

	expr_mutate_builtin(fcall, unreachable);
	fcall->f_gen = builtin_gen_undefined;

	return fcall;
}

/* --- compatible_p */

static void fold_compatible_p(expr *e, symtable *stab)
{
	decl **types = e->block_args->arglist;

	if(dynarray_count((void **)types) != 2)
		DIE_AT(&e->where, "need two arguments for %s", e->expr->spel);

	fold_decl(types[0], stab);
	fold_decl(types[1], stab);

	e->tree_type = decl_new_int();
	wur_builtin(e);
}

static void const_compatible_p(expr *e, intval *val, enum constyness *success)
{
	decl **types = e->block_args->arglist;

	*success = CONST_WITH_VAL;

	val->val = decl_equal(types[0], types[1], DECL_CMP_STRICT_PRIMITIVE);
}

static expr *parse_compatible_p(void)
{
	expr *fcall = expr_new_funcall();

	fcall->block_args = funcargs_new();
	fcall->block_args->arglist = parse_type_list();

	expr_mutate_builtin_no_gen(fcall, compatible_p);

	return fcall;
}

/* --- constant */

static void fold_constant_p(expr *e, symtable *stab)
{
	if(dynarray_count((void **)e->funcargs) != 1)
		DIE_AT(&e->where, "%s takes a single argument", e->expr->spel);

	fold_expr(e->funcargs[0], stab);

	e->tree_type = decl_new_int();
	wur_builtin(e);
}

static void const_constant_p(expr *e, intval *val, enum constyness *success)
{
	expr *test = *e->funcargs;
	enum constyness type;
	intval iv;

	const_fold(test, &iv, &type);

	*success = CONST_WITH_VAL;
	val->val = type != CONST_NO;
}

static expr *parse_constant_p(void)
{
	expr *fcall = parse_any_args();
	expr_mutate_builtin_no_gen(fcall, constant_p);
	return fcall;
}

/* --- frame_address */

static void fold_frame_address(expr *e, symtable *stab)
{
	enum constyness type;
	intval iv;

	if(dynarray_count((void **)e->funcargs) != 1)
		DIE_AT(&e->where, "%s takes a single argument", e->expr->spel);

	fold_expr(e->funcargs[0], stab);

	const_fold(e->funcargs[0], &iv, &type);
	if(type != CONST_WITH_VAL || iv.val < 0)
		DIE_AT(&e->where, "%s needs a positive constant value argument", e->expr->spel);

	memcpy(&e->val.iv, &iv, sizeof iv);

	e->tree_type = decl_ptr_depth_inc(decl_new_void());
	wur_builtin(e);
}

static void builtin_gen_frame_pointer(expr *e, symtable *stab)
{
	const int depth = e->val.iv.val;

	(void)stab;

	out_push_frame_ptr(depth + 1);
}

static expr *parse_frame_address(void)
{
	expr *fcall = parse_any_args();
	expr_mutate_builtin(fcall, frame_address);
	fcall->f_gen = builtin_gen_frame_pointer;
	return fcall;
}
