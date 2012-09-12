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
													parse_trap,
													parse_compatible_p,
													parse_constant_p;

static func_fold fold_unreachable,
								 fold_trap,
								 fold_compatible_p,
								 fold_constant_p;

static func_const const_compatible_p,
									const_constant_p;


typedef struct
{
	const char *sp;
	func_builtin_parse *parser;
} builtin_table;

builtin_table builtins[] = {
	{ "unreachable", parse_unreachable },
	{ "trap", parse_trap },

	{ "types_compatible_p", parse_compatible_p },
	{ "constant_p", parse_constant_p },

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

#define expr_mutate_builtin(exp, to)     \
	exp->f_fold       = fold_ ## to

#define expr_mutate_builtin_const(exp, to) \
	expr_mutate_builtin(exp, to),            \
	exp->f_gen        = builtin_gen_const,   \
	exp->f_const_fold = const_ ## to

static void builtin_gen_const(expr *e, symtable *stab)
{
	intval iv;

	const_fold_need_val(e, &iv);

	(void)stab;

	out_push_i(NULL, iv.val);
}

static void builtin_gen_undefined(expr *e, symtable *stab)
{
	(void)e;
	(void)stab;
	out_undefined();
	out_push_i(NULL, 0);
}

/* --- unreachable */

static expr *parse_unreachable(void)
{
	expr *fcall = expr_new_funcall();

	expr_mutate_builtin(fcall, unreachable);
	fcall->f_gen = builtin_gen_undefined;

	return fcall;
}

static void fold_unreachable(expr *e, symtable *stab)
{
	(void)stab;

	e->tree_type = decl_new_void();
	decl_attr_append(&e->tree_type->attr, decl_attr_new(attr_noreturn));
}

/* --- trap */

static expr *parse_trap(void)
{
	expr *fcall = expr_new_funcall();

	expr_mutate_builtin(fcall, trap);
	fcall->f_gen = builtin_gen_undefined;

	return fcall;
}

static void fold_trap(expr *e, symtable *stab)
{
	(void)stab;
	e->tree_type = decl_new_void();
}

/* --- compatible_p */

static expr *parse_compatible_p(void)
{
	expr *fcall = expr_new_funcall();

	fcall->block_args = funcargs_new();
	fcall->block_args->arglist = parse_type_list();

	expr_mutate_builtin_const(fcall, compatible_p);

	return fcall;
}

static void fold_compatible_p(expr *e, symtable *stab)
{
	decl **types = e->block_args->arglist;

	(void)stab;

	if(dynarray_count((void **)types) != 2)
		DIE_AT(&e->where, "need two arguments for %s", e->expr->spel);

	fold_decl(types[0], stab);
	fold_decl(types[1], stab);

	e->tree_type = decl_new_int();
}

static void const_compatible_p(expr *e, intval *val, enum constyness *success)
{
	decl **types = e->block_args->arglist;

	*success = CONST_WITH_VAL;

	val->val = decl_equal(types[0], types[1], DECL_CMP_STRICT_PRIMITIVE);
}

/* --- constant */

static expr *parse_constant_p(void)
{
	expr *fcall = expr_new_funcall();

	fcall->funcargs = parse_funcargs();

	expr_mutate_builtin_const(fcall, constant_p);

	return fcall;
}

static void fold_constant_p(expr *e, symtable *stab)
{
	(void)stab;

	if(dynarray_count((void **)e->funcargs) != 1)
		DIE_AT(&e->where, "%s takes a single argument", e->expr->spel);

	fold_expr(e->funcargs[0], stab);

	e->tree_type = decl_new_int();
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
