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
#include "../funcargs.h"

#include "../const.h"
#include "../gen_asm.h"

#include "../out/out.h"

#define PREFIX "__builtin_"

#define BUILTIN_SPEL(e) (e)->bits.ident.spel

typedef expr *func_builtin_parse(void);

static func_builtin_parse parse_unreachable,
                          parse_compatible_p,
                          parse_constant_p,
                          parse_frame_address,
                          parse_expect,
                          parse_strlen,
													parse_is_signed;

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

	{ "expect", parse_expect },

	{ "is_signed", parse_is_signed },

	{ NULL, NULL }

}, no_prefix_builtins[] = {
	{ "strlen", parse_strlen },

	{ NULL, NULL }
};

static builtin_table *builtin_table_search(builtin_table *tab, const char *sp)
{
	int i;
	for(i = 0; tab[i].sp; i++)
		if(!strcmp(sp, tab[i].sp))
			return &tab[i];
	return NULL;
}

static builtin_table *builtin_find(const char *sp)
{
	static int prefix_len;
	builtin_table *tab;

	if(!strncmp(sp, PREFIX, prefix_len)){
		if(!prefix_len)
			prefix_len = strlen(PREFIX);

		sp += prefix_len;
		tab = builtins;
	}else{
		tab = no_prefix_builtins;
	}

	return builtin_table_search(tab, sp);
}

expr *builtin_parse(const char *sp)
{
	builtin_table *b;

	if((fopt_mode & FOPT_BUILTIN) && (b = builtin_find(sp))){
		expr *(*f)(void) = b->parser;

		if(f)
			return f();
	}

	return NULL;
}

#define expr_mutate_builtin(exp, to)  \
	exp->f_fold = fold_ ## to

#define expr_mutate_builtin_const(exp, to) \
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
	out_push_i(type_ref_new_INT(), 0); /* needed for function return pop */
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

	e->tree_type = type_ref_new_type(type_new_primitive(type_void));
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
		DIE_AT(&e->where, "need two arguments for %s", BUILTIN_SPEL(e->expr));

	fold_decl(types[0], stab);
	fold_decl(types[1], stab);

	e->tree_type = type_ref_new_BOOL();
	wur_builtin(e);
}

static void const_compatible_p(expr *e, consty *k)
{
	type_ref **types = e->bits.types;

	k->type = CONST_VAL;

	k->bits.iv.val = type_ref_equal(types[0], types[1], DECL_CMP_EXACT_MATCH);
}

static expr *expr_new_funcall_typelist(void)
{
	expr *fcall = expr_new_funcall();

	fcall->bits.types = parse_type_list();

	return fcall;
}

static expr *parse_compatible_p(void)
{
	expr *fcall = expr_new_funcall_typelist();

	expr_mutate_builtin_const(fcall, compatible_p);

	return fcall;
}

/* --- constant */

static void fold_constant_p(expr *e, symtable *stab)
{
	if(dynarray_count((void **)e->funcargs) != 1)
		DIE_AT(&e->where, "%s takes a single argument", BUILTIN_SPEL(e->expr));

	FOLD_EXPR(e->funcargs[0], stab);

	e->tree_type = type_ref_new_BOOL();
	wur_builtin(e);
}

static void const_constant_p(expr *e, consty *k)
{
	expr *test = *e->funcargs;
	consty subk;

	const_fold(test, &subk);

	k->type = CONST_VAL;
	k->bits.iv.val = is_const(subk.type);
}

static expr *parse_constant_p(void)
{
	expr *fcall = parse_any_args();
	expr_mutate_builtin_const(fcall, constant_p);
	return fcall;
}

/* --- frame_address */

static void fold_frame_address(expr *e, symtable *stab)
{
	consty k;

	if(dynarray_count((void **)e->funcargs) != 1)
		DIE_AT(&e->where, "%s takes a single argument", BUILTIN_SPEL(e->expr));

	FOLD_EXPR(e->funcargs[0], stab);

	const_fold(e->funcargs[0], &k);
	if(k.type != CONST_VAL || k.bits.iv.val < 0)
		DIE_AT(&e->where, "%s needs a positive constant value argument", BUILTIN_SPEL(e->expr));

	memcpy_safe(&e->bits.iv, &k.bits.iv);

	e->tree_type = type_ref_new_ptr(
			type_ref_new_type(
				type_new_primitive(type_void)
			),
			qual_none);

	wur_builtin(e);
}

static void builtin_gen_frame_pointer(expr *e, symtable *stab)
{
	const int depth = e->bits.iv.val;

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

/* --- expect */

static void fold_expect(expr *e, symtable *stab)
{
	consty k;
	int i;

	if(dynarray_count((void **)e->funcargs) != 2)
		DIE_AT(&e->where, "%s takes two arguments", BUILTIN_SPEL(e->expr));

	for(i = 0; i < 2; i++)
		FOLD_EXPR(e->funcargs[i], stab);

	const_fold(e->funcargs[1], &k);
	if(k.type != CONST_VAL)
		WARN_AT(&e->where, "%s second argument isn't a constant value", BUILTIN_SPEL(e->expr));

	e->tree_type = e->funcargs[0]->tree_type;
	wur_builtin(e);
}

static void builtin_gen_expect(expr *e, symtable *stab)
{
	gen_expr(e->funcargs[1], stab); /* not needed if it's const, but gcc and clang do this */
	out_pop();
	gen_expr(e->funcargs[0], stab);
}

static void const_expect(expr *e, consty *k)
{
	/* forward on */
	const_fold(e->funcargs[0], k);
}

static expr *parse_expect(void)
{
	expr *fcall = parse_any_args();
	expr_mutate_builtin_const(fcall, expect);
	fcall->f_gen = builtin_gen_expect;
	return fcall;
}

/* --- is_signed */

static void fold_is_signed(expr *e, symtable *stab)
{
	type_ref **tl = e->bits.types;

	if(dynarray_count((void **)tl) != 1)
		DIE_AT(&e->where, "need a single argument for %s", BUILTIN_SPEL(e->expr));

	fold_type_ref(tl[0], NULL, stab);

	e->tree_type = type_ref_new_BOOL();
	wur_builtin(e);
}

static void const_is_signed(expr *e, consty *k)
{
	memset(k, 0, sizeof *k);
	k->type = CONST_VAL;
	k->bits.iv.val = type_ref_is_signed(e->bits.types[0]);
}

static expr *parse_is_signed(void)
{
	expr *fcall = expr_new_funcall_typelist();

	/* simply set the const vtable ent */
	fcall->f_fold       = fold_is_signed;
	fcall->f_const_fold = const_is_signed;

	return fcall;
}

/* --- strlen */

static void const_strlen(expr *e, consty *k)
{
	k->type = CONST_NO;

	/* if 1 arg and it has a char * constant, return length */
	if(dynarray_count((void **)e->funcargs) == 1){
		expr *s = e->funcargs[0];
		consty subk;

		const_fold(s, &subk);
		if(subk.type == CONST_STRK){
			stringval *sv = subk.bits.str;
			const char *s = sv->str;
			const char *p = memchr(s, '\0', sv->len);

			if(p){
				k->type = CONST_VAL;
				k->bits.iv.val = p - s;
				k->bits.iv.suffix = VAL_UNSIGNED;
			}
		}
	}
}

static expr *parse_strlen(void)
{
	expr *fcall = parse_any_args();

	/* simply set the const vtable ent */
	fcall->f_const_fold = const_strlen;

	return fcall;
}
