#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "../../util/util.h"
#include "../../util/dynarray.h"
#include "../../util/alloc.h"

#include "__builtin.h"

#include "../cc1.h"
#include "../tokenise.h"
#include "../fold.h"
#include "../funcargs.h"
#include "../type_nav.h"
#include "../type_is.h"

#include "../const.h"
#include "../gen_asm.h"
#include "../gen_str.h"

#include "../out/out.h"
#include "__builtin_va.h"

#include "../parse_expr.h"
#include "../parse_type.h"

#define PREFIX "__builtin_"

typedef expr *func_builtin_parse(
		const char *ident, symtable *);

static func_builtin_parse parse_unreachable,
                          parse_compatible_p,
                          parse_constant_p,
                          parse_frame_address,
                          parse_expect,
                          parse_strlen,
                          parse_is_signed,
                          parse_nan,
                          parse_choose_expr;
#ifdef BUILTIN_LIBC_FUNCTIONS
                          parse_memset,
                          parse_memcpy;
#endif

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

	{ "nan",  parse_nan },
	{ "nanf", parse_nan },
	{ "nanl", parse_nan },

	{ "choose_expr", parse_choose_expr },

#define BUILTIN_VA(nam) { "va_" #nam, parse_va_ ##nam },
#  include "__builtin_va.def"
#undef BUILTIN_VA

	{ NULL, NULL }

}, no_prefix_builtins[] = {
	{ "strlen", parse_strlen },
#ifdef BUILTIN_LIBC_FUNCTIONS
	{ "memset", parse_memset },
	{ "memcpy", parse_memcpy },
#endif

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
	static unsigned prefix_len;
	builtin_table *found;

	found = builtin_table_search(no_prefix_builtins, sp);
	if(found)
		return found;

	if(!prefix_len)
		prefix_len = strlen(PREFIX);

	if(strlen(sp) < prefix_len)
		return NULL;

	if(!strncmp(sp, PREFIX, prefix_len))
		found = builtin_table_search(builtins, sp + prefix_len);
	else
		found = NULL;

	if(!found) /* look for __builtin_strlen, etc */
		found = builtin_table_search(no_prefix_builtins, sp + prefix_len);

	return found;
}

expr *builtin_parse(const char *sp, symtable *scope)
{
	builtin_table *b;

	if((fopt_mode & FOPT_BUILTIN) && (b = builtin_find(sp))){
		expr *(*f)(const char *, symtable *) = b->parser;

		if(f)
			return f(sp, scope);
	}

	return NULL;
}

const out_val *builtin_gen_print(expr *e, out_ctx *octx)
{
	/*const enum pdeclargs dflags =
		  PDECL_INDENT
		| PDECL_NEWLINE
		| PDECL_SYM_OFFSET
		| PDECL_FUNC_DESCEND
		| PDECL_PISDEF
		| PDECL_PINIT
		| PDECL_SIZE
		| PDECL_ATTR;*/

	idt_printf("%s(\n", BUILTIN_SPEL(e->expr));

#define PRINT_ARGS(type, from, func)      \
	{                                       \
		type **i;                             \
                                          \
		gen_str_indent++;                     \
		for(i = from; i && *i; i++){          \
			func;                               \
			if(i[1])                            \
				idt_printf(",\n");                \
		}                                     \
		gen_str_indent--;                     \
	}

	if(e->funcargs)
		PRINT_ARGS(expr, e->funcargs, print_expr(*i))

	/*if(e->bits.block_args)
		PRINT_ARGS(decl, e->bits.block_args->arglist, print_decl(*i, dflags))*/

	idt_printf(");\n");

	(void)octx;
	return NULL;
}

#define expr_mutate_builtin_const(exp, to) \
	expr_mutate_builtin(exp, to),             \
	exp->f_const_fold = const_ ## to

static void wur_builtin(expr *e)
{
	e->freestanding = 0; /* needs use */
}

static const out_val *builtin_gen_undefined(expr *e, out_ctx *octx)
{
	(void)e;
	out_ctrl_end_undefined(octx);
	return out_new_noop(octx);
}

expr *parse_any_args(symtable *scope)
{
	expr *fcall = expr_new_funcall();

	fcall->funcargs = parse_funcargs(scope);
	return fcall;
}

/* --- memset */

static void fold_memset(expr *e, symtable *stab)
{
	fold_expr_no_decay(e->lhs, stab);

	if(!expr_is_addressable(e->lhs))
		ICE("can't memset %s - not addressable", e->lhs->f_str());

	if(e->bits.builtin_memset.len == 0)
		warn_at(&e->where, "zero size memset");

	if((unsigned)e->bits.builtin_memset.ch > 255)
		warn_at(&e->where, "memset with value > UCHAR_MAX");

	e->tree_type = type_ptr_to(type_nav_btype(cc1_type_nav, type_void));
}

static const out_val *builtin_gen_memset(expr *e, out_ctx *octx)
{
	size_t n, rem;
	unsigned i;
	type *tzero = type_nav_MAX_FOR(
			cc1_type_nav,
			e->bits.builtin_memset.len);

	type *textra, *textrap;
	const out_val *v_ptr;

	if(!tzero)
		tzero = type_nav_btype(cc1_type_nav, type_nchar);

	n   = e->bits.builtin_memset.len / type_size(tzero, NULL);
	rem = e->bits.builtin_memset.len % type_size(tzero, NULL);

	if((textra = rem ? type_nav_MAX_FOR(cc1_type_nav, rem) : NULL))
		textrap = type_ptr_to(textra);

	/* works fine for bitfields - struct lea acts appropriately */
	v_ptr = lea_expr(e->lhs, octx);

	v_ptr = out_change_type(octx, v_ptr, type_ptr_to(tzero));

#ifdef MEMSET_VERBOSE
	out_comment("memset(%s, %d, %lu), using ptr<%s>, %lu steps",
			e->expr->f_str(),
			e->bits.builtin_memset.ch,
			e->bits.builtin_memset.len,
			type_to_str(tzero), n);
#endif

	for(i = 0; i < n; i++){
		const out_val *v_zero = out_new_zero(octx, tzero);
		const out_val *v_inc;

		/* *p = 0 */
		out_val_retain(octx, v_ptr);
		out_store(octx, v_ptr, v_zero);

		/* p++ (copied pointer) */
		v_inc = out_new_l(octx, type_nav_btype(cc1_type_nav, type_intptr_t), 1);

		v_ptr = out_op(octx, op_plus, v_ptr, v_inc);

		if(rem){
			/* need to zero a little more */
			v_ptr = out_change_type(octx, v_ptr, textrap);
			v_zero = out_new_zero(octx, textra);

			out_val_retain(octx, v_ptr);
			out_store(octx, v_ptr, v_zero);
		}
	}

	return out_op(
			octx, op_minus,
			v_ptr,
			out_new_l(octx, e->tree_type, e->bits.builtin_memset.len));
}

expr *builtin_new_memset(expr *p, int ch, size_t len)
{
	expr *fcall = expr_new_funcall();

	fcall->expr = expr_new_identifier("__builtin_memset");

	expr_mutate_builtin_gen(fcall, memset);

	fcall->lhs = p;
	fcall->bits.builtin_memset.ch = ch;
	UCC_ASSERT(ch == 0, "TODO: non-zero memset");
	fcall->bits.builtin_memset.len = len;

	return fcall;
}

#ifdef BUILTIN_LIBC_FUNCTIONS
static expr *parse_memset(void)
{
	expr *fcall = parse_any_args();

	ICE("TODO: builtin memset parsing");

	expr_mutate_builtin_gen(fcall, memset);

	return fcall;
}
#endif

/* --- memcpy */

static void fold_memcpy(expr *e, symtable *stab)
{
	fold_expr_no_decay(e->lhs, stab);
	fold_expr_no_decay(e->rhs, stab);

	e->tree_type = type_ptr_to(type_nav_btype(cc1_type_nav, type_void));
}

#ifdef BUILTIN_USE_LIBC
static decl *decl_new_tref(char *sp, type *ref)
{
	decl *d = decl_new();
	d->ref = ref;
	d->spel = sp;
	return d;
}
#endif

static void builtin_memcpy_single(
		out_ctx *octx,
		const out_val **dst, const out_val **src)
{
	type *t1 = type_nav_btype(cc1_type_nav, type_intptr_t);

	out_store(octx, *dst, out_deref(octx, *src));

	*dst = out_op(octx, op_plus, *dst, out_new_l(octx, t1, 1));
	*src = out_op(octx, op_plus, *src, out_new_l(octx, t1, 1));
}

static const out_val *builtin_gen_memcpy(expr *e, out_ctx *octx)
{
#ifdef BUILTIN_USE_LIBC
	/* TODO - also with memset */
	funcargs *fargs = funcargs_new();

	dynarray_add(&fargs->arglist, decl_new_tref(NULL, type_cached_VOID_PTR()));
	dynarray_add(&fargs->arglist, decl_new_tref(NULL, type_cached_VOID_PTR()));
	dynarray_add(&fargs->arglist, decl_new_tref(NULL, type_cached_INTPTR_T()));

	type *ctype = type_new_func(
			e->tree_type, fargs);

	out_push_lbl("memcpy", 0);
	out_push_l(type_cached_INTPTR_T(), e->bits.num.val);
	lea_expr(e->rhs, stab);
	lea_expr(e->lhs, stab);
	out_call(3, e->tree_type, ctype);
#else
	size_t i = e->bits.num.val.i;
	type *tptr;
	unsigned tptr_sz;
	const out_val *dest, *src;

	dest = lea_expr(e->lhs, octx);
	src = lea_expr(e->rhs, octx);

	if(i > 0){
		tptr = type_ptr_to(type_nav_MAX_FOR(cc1_type_nav, e->bits.num.val.i));
		tptr_sz = type_size(tptr, &e->where);
	}

	while(i > 0){
		/* as many copies as we can */
		dest = out_change_type(octx, dest, tptr);
		src = out_change_type(octx, src, tptr);

		while(i >= tptr_sz){
			i -= tptr_sz;
			builtin_memcpy_single(octx, &dest, &src);
		}

		if(i > 0){
			tptr_sz /= 2;
			tptr = type_ptr_to(type_nav_MAX_FOR(cc1_type_nav, tptr_sz));
		}
	}

	return out_op(
			octx, op_minus,
			dest, out_new_l(octx, e->tree_type, e->bits.num.val.i));
#endif
}

expr *builtin_new_memcpy(expr *to, expr *from, size_t len)
{
	expr *fcall = expr_new_funcall();

	fcall->expr = expr_new_identifier("__builtin_memcpy");

	expr_mutate_builtin(fcall, memcpy);
	BUILTIN_SET_GEN(fcall, builtin_gen_memcpy);

	fcall->lhs = to;
	fcall->rhs = from;
	fcall->bits.num.val.i = len;

	return fcall;
}

#ifdef BUILTIN_LIBC_FUNCTIONS
static expr *parse_memcpy(void)
{
	expr *fcall = parse_any_args();

	ICE("TODO: builtin memcpy parsing");

	expr_mutate_builtin_gen(fcall, memcpy);

	return fcall;
}
#endif

/* --- unreachable */

static void fold_unreachable(expr *e, symtable *stab)
{
	type *tvoid = type_nav_btype(cc1_type_nav , type_void);

	e->expr->tree_type = type_func_of(tvoid,
			funcargs_new(), symtab_new(stab, &e->where));

	e->tree_type = type_attributed(tvoid, attribute_new(attr_noreturn));

	wur_builtin(e);
}

static expr *parse_unreachable(const char *ident, symtable *scope)
{
	expr *fcall = expr_new_funcall();

	(void)ident;
	(void)scope;

	expr_mutate_builtin(fcall, unreachable);

	BUILTIN_SET_GEN(fcall, builtin_gen_undefined);

	return fcall;
}

/* --- compatible_p */

static void fold_compatible_p(expr *e, symtable *stab)
{
	type **types = e->bits.types;

	if(dynarray_count(types) != 2)
		die_at(&e->where, "need two arguments for %s", BUILTIN_SPEL(e->expr));

	fold_type(types[0], stab);
	fold_type(types[1], stab);

	e->tree_type = type_nav_btype(cc1_type_nav, type__Bool);
	wur_builtin(e);
}

static void const_compatible_p(expr *e, consty *k)
{
	type **types = e->bits.types;

	CONST_FOLD_LEAF(k);

	k->type = CONST_NUM;

	k->bits.num.val.i = !!(type_cmp(types[0], types[1], 0) & TYPE_EQUAL_ANY);
}

static expr *expr_new_funcall_typelist(symtable *scope)
{
	expr *fcall = expr_new_funcall();

	fcall->bits.types = parse_type_list(scope);

	return fcall;
}

static expr *parse_compatible_p(const char *ident, symtable *scope)
{
	expr *fcall = expr_new_funcall_typelist(scope);

	(void)ident;

	expr_mutate_builtin_const(fcall, compatible_p);

	return fcall;
}

/* --- constant */

static void fold_constant_p(expr *e, symtable *stab)
{
	if(dynarray_count(e->funcargs) != 1)
		die_at(&e->where, "%s takes a single argument", BUILTIN_SPEL(e->expr));

	FOLD_EXPR(e->funcargs[0], stab);

	e->tree_type = type_nav_btype(cc1_type_nav, type__Bool);
	wur_builtin(e);
}

static void const_constant_p(expr *e, consty *k)
{
	expr *test = *e->funcargs;
	int is_const;

	const_fold(test, k);

	is_const = CONST_AT_COMPILE_TIME(k->type);

	CONST_FOLD_LEAF(k);
	k->type = CONST_NUM;
	k->bits.num.val.i = is_const;
}

static expr *parse_constant_p(const char *ident, symtable *scope)
{
	expr *fcall = parse_any_args(scope);
	(void)ident;

	expr_mutate_builtin_const(fcall, constant_p);
	return fcall;
}

/* --- frame_address */

static void fold_frame_address(expr *e, symtable *stab)
{
	consty k;

	if(dynarray_count(e->funcargs) != 1)
		die_at(&e->where, "%s takes a single argument", BUILTIN_SPEL(e->expr));

	FOLD_EXPR(e->funcargs[0], stab);

	const_fold(e->funcargs[0], &k);
	if(k.type != CONST_NUM
	|| (K_FLOATING(k.bits.num))
	|| (sintegral_t)k.bits.num.val.i < 0)
	{
		die_at(&e->where, "%s needs a positive integral constant value argument", BUILTIN_SPEL(e->expr));
	}

	memcpy_safe(&e->bits.num, &k.bits.num);

	e->tree_type = type_ptr_to(type_nav_btype(cc1_type_nav, type_nchar));

	wur_builtin(e);
}

static const out_val *builtin_gen_frame_address(expr *e, out_ctx *octx)
{
	const int depth = e->bits.num.val.i;

	return out_new_frame_ptr(octx, depth + 1);
}

static expr *builtin_frame_address_mutate(expr *e)
{
	expr_mutate_builtin(e, frame_address);
	BUILTIN_SET_GEN(e, builtin_gen_frame_address);
	return e;
}

static expr *parse_frame_address(const char *ident, symtable *scope)
{
	(void)ident;

	return builtin_frame_address_mutate(parse_any_args(scope));
}

expr *builtin_new_frame_address(int depth)
{
	expr *e = expr_new_funcall();

	dynarray_add(&e->funcargs, expr_new_val(depth));

	return builtin_frame_address_mutate(e);
}

/* --- reg_save_area (a basic wrapper around out_push_reg_save_ptr()) */

static void fold_reg_save_area(expr *e, symtable *stab)
{
	(void)stab;
	e->tree_type = type_ptr_to(type_nav_btype(cc1_type_nav, type_nchar));
}

static const out_val *gen_reg_save_area(expr *e, out_ctx *octx)
{
	(void)e;
	out_comment(octx, "stack local offset:");
	return out_new_reg_save_ptr(octx);
}

expr *builtin_new_reg_save_area(void)
{
	expr *e = expr_new_funcall();

	expr_mutate_builtin(e, reg_save_area);
	BUILTIN_SET_GEN(e, gen_reg_save_area);

	return e;
}

/* --- expect */

static void fold_expect(expr *e, symtable *stab)
{
	consty k;
	int i;

	if(dynarray_count(e->funcargs) != 2)
		die_at(&e->where, "%s takes two arguments", BUILTIN_SPEL(e->expr));

	for(i = 0; i < 2; i++)
		FOLD_EXPR(e->funcargs[i], stab);

	const_fold(e->funcargs[1], &k);
	if(k.type != CONST_NUM)
		warn_at(&e->where, "%s second argument isn't a constant value", BUILTIN_SPEL(e->expr));

	e->tree_type = e->funcargs[0]->tree_type;
	wur_builtin(e);
}

static const out_val *builtin_gen_expect(expr *e, out_ctx *octx)
{
	/* not needed if it's const, but gcc and clang do this */
	out_val_consume(octx,
			gen_expr(e->funcargs[1], octx));

	return gen_expr(e->funcargs[0], octx);
}

static void const_expect(expr *e, consty *k)
{
	/* forward on */
	const_fold(e->funcargs[0], k);
}

static expr *parse_expect(const char *ident, symtable *scope)
{
	expr *fcall = parse_any_args(scope);

	(void)ident;

	expr_mutate_builtin_const(fcall, expect);
	BUILTIN_SET_GEN(fcall, builtin_gen_expect);
	return fcall;
}

/* --- choose_expr */

#define CHOOSE_EXPR_CHOSEN(e) ((e)->funcargs[(e)->bits.num.val.i ? 1 : 2])

static const out_val *choose_expr_lea(expr *e, out_ctx *octx)
{
	return lea_expr(CHOOSE_EXPR_CHOSEN(e), octx);
}

static void fold_choose_expr(expr *e, symtable *stab)
{
	consty k;
	int i;
	expr *c;

	if(dynarray_count(e->funcargs) != 3)
		die_at(&e->where, "three arguments expected for %s",
				BUILTIN_SPEL(e->expr));

	for(i = 0; i < 3; i++)
		fold_expr_no_decay(e->funcargs[i], stab);

	const_fold(e->funcargs[0], &k);
	if(k.type != CONST_NUM){
		die_at(&e->funcargs[0]->where,
				"first argument to %s not constant",
				BUILTIN_SPEL(e->expr));
	}

	i = !!(K_INTEGRAL(k.bits.num) ? k.bits.num.val.i : k.bits.num.val.f);
	e->bits.num.val.i = i;

	c = CHOOSE_EXPR_CHOSEN(e);
	e->tree_type = c->tree_type;

	wur_builtin(e);

	if(expr_is_lval(c))
		e->f_lea = choose_expr_lea;
}

static void const_choose_expr(expr *e, consty *k)
{
	/* forward to the chosen expr */
	const_fold(CHOOSE_EXPR_CHOSEN(e), k);
}

static const out_val *gen_choose_expr(expr *e, out_ctx *octx)
{
	/* forward to the chosen expr */
	return gen_expr(CHOOSE_EXPR_CHOSEN(e), octx);
}

static expr *parse_choose_expr(const char *ident, symtable *scope)
{
	expr *fcall = parse_any_args(scope);

	(void)ident;

	fcall->f_fold       = fold_choose_expr;
	fcall->f_const_fold = const_choose_expr;
	BUILTIN_SET_GEN(fcall, gen_choose_expr);

	return fcall;
}

/* --- is_signed */

static void fold_is_signed(expr *e, symtable *stab)
{
	type **tl = e->bits.types;
	int incomplete;

	if(dynarray_count(tl) != 1)
		die_at(&e->where, "need a single argument for %s", BUILTIN_SPEL(e->expr));

	fold_type(tl[0], stab);

	if((incomplete = !type_is_complete(tl[0]))
	|| !type_is_scalar(tl[0]))
	{
		warn_at_print_error(&e->where, "%s on %s type '%s'",
				BUILTIN_SPEL(e->expr),
				incomplete ? "incomplete" : "non-scalar",
				type_to_str(tl[0]));

		fold_had_error = 1;
	}

	e->tree_type = type_nav_btype(cc1_type_nav, type__Bool);
	wur_builtin(e);
}

static void const_is_signed(expr *e, consty *k)
{
	CONST_FOLD_LEAF(k);
	k->type = CONST_NUM;
	k->bits.num.val.i = type_is_signed(e->bits.types[0]);
}

static expr *parse_is_signed(const char *ident, symtable *scope)
{
	expr *fcall = expr_new_funcall_typelist(scope);

	(void)ident;

	/* simply set the const vtable ent */
	fcall->f_fold       = fold_is_signed;
	fcall->f_const_fold = const_is_signed;

	return fcall;
}

/* --- nan */

static void fold_nan(expr *e, symtable *stab)
{
	enum type_primitive prim = type_void;
	consty k;

	if(dynarray_count(e->funcargs) != 1){
need_char_p:
		die_at(&e->where, "%s takes a single 'char *' argument",
				BUILTIN_SPEL(e->expr));
	}

	FOLD_EXPR(e->funcargs[0], stab);

	const_fold(e->funcargs[0], &k);
	if(k.type != CONST_STRK)
		goto need_char_p;

	if(!stringlit_empty(k.bits.str->lit))
		die_at(&e->where, "only implemented for %s(\"\")",
				BUILTIN_SPEL(e->expr));

	switch(e->bits.builtin_nantype){
		case builtin_nanf: prim = type_float; break;
		case builtin_nan:  prim = type_double; break;
		case builtin_nanl: prim = type_ldouble; break;
	}
	e->tree_type = type_nav_btype(cc1_type_nav, prim);

	wur_builtin(e);
}

static const out_val *builtin_gen_nan(expr *e, out_ctx *octx)
{
	return out_new_nan(octx, e->tree_type);
}

static expr *builtin_nan_mutate(expr *e)
{
	expr_mutate_builtin(e, nan);
	BUILTIN_SET_GEN(e, builtin_gen_nan);
	return e;
}

static expr *parse_nan(const char *ident, symtable *scope)
{
	expr *e = builtin_nan_mutate(parse_any_args(scope));

	if(!strncmp(ident, PREFIX, strlen(PREFIX)))
		ident += strlen(PREFIX);

	if(!strcmp(ident, "nan"))
		e->bits.builtin_nantype = builtin_nan;
	else if(!strcmp(ident, "nanf"))
		e->bits.builtin_nantype = builtin_nanf;
	else if(!strcmp(ident, "nanl"))
		e->bits.builtin_nantype = builtin_nanl;
	else
		ICE("bad nantype '%s'", ident);

	return e;
}

/* --- strlen */

static void const_strlen(expr *e, consty *k)
{
	k->type = CONST_NO;

	/* if 1 arg and it has a char * constant, return length */
	if(dynarray_count(e->funcargs) == 1){
		expr *s = e->funcargs[0];
		consty subk;

		const_fold(s, &subk);
		if(subk.type == CONST_STRK){
			stringlit *lit = subk.bits.str->lit;
			const char *s = lit->str;
			const char *p = memchr(s, '\0', lit->len);

			if(p){
				CONST_FOLD_LEAF(k);
				k->type = CONST_NUM;
				k->bits.num.val.i = p - s;
				k->bits.num.suffix = VAL_UNSIGNED;
			}
		}
	}
}

static expr *parse_strlen(const char *ident, symtable *scope)
{
	expr *fcall = parse_any_args(scope);

	(void)ident;

	/* simply set the const vtable ent */
	fcall->f_const_fold = const_strlen;

	return fcall;
}
