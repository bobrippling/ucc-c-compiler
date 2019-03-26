#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>

#include "../../util/util.h"
#include "../../util/dynarray.h"
#include "../../util/alloc.h"

#include "__builtin.h"

#include "../cc1.h"
#include "../fopt.h"
#include "../tokenise.h"
#include "../fold.h"
#include "../funcargs.h"
#include "../type_nav.h"
#include "../type_is.h"
#include "../tokconv.h"
#include "../str.h"

#include "../const.h"
#include "../gen_asm.h"
#include "../gen_dump.h"

#include "../out/out.h"
#include "__builtins.h"
#include "__builtin_va.h"

#include "../parse_expr.h"
#include "../parse_type.h"
#include "../cc1_out_ctx.h"

#include "../ops/expr_funcall.h"
#include "../ops/expr_addr.h"
#include "../ops/expr_identifier.h"
#include "../ops/expr_val.h"
#include "../ops/expr_struct.h"
#include "../ops/expr_cast.h"
#include "../ops/expr_op.h"

#define PREFIX "__builtin_"

typedef expr *func_builtin_parse(
		const char *ident, symtable *);

static func_builtin_parse parse_unreachable
                          , parse_compatible_p
                          , parse_constant_p
                          , parse_frame_address
                          , parse_return_address
                          , parse_extract_return_addr
                          , parse_expect
                          , parse_strlen
                          , parse_is_signed
                          , parse_nan
                          , parse_choose_expr
                          , parse_offsetof
                          , parse_has_attribute
                          , parse_add_overflow
                          , parse_sub_overflow
                          , parse_mul_overflow
#ifdef BUILTIN_LIBC_FUNCTIONS
                          , parse_memset
                          , parse_memcpy
#endif
                          ;

typedef struct
{
	const char *sp;
	func_builtin_parse *parser;
} builtin_table;

builtin_table builtins[] = {
#define BUILTIN(name, type) { name, parse_ ## type },
	BUILTINS
#undef BUILTIN

	{ NULL, NULL }

}, no_prefix_builtins[] = {
	{ "strlen", parse_strlen },
#ifdef BUILTIN_LIBC_FUNCTIONS
	{ "memset", parse_memset },
	{ "memcpy", parse_memcpy },
#endif

	{ NULL, NULL }
};


#define expr_mutate_builtin_const(exp, to) \
	exp->f_fold = fold_ ## to,               \
	exp->f_const_fold = const_ ## to


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

	if(!cc1_fopt.freestanding){
		found = builtin_table_search(no_prefix_builtins, sp);
		if(found)
			return found;
	}

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

	if(cc1_fopt.builtin && (b = builtin_find(sp))){
		expr *(*f)(const char *, symtable *) = b->parser;

		if(f)
			return f(sp, scope);
	}

	return NULL;
}

static void wur_builtin(expr *e)
{
	e->freestanding = 0; /* needs use */
}

const char *str_expr_builtin(void)
{
	return "<builtin-expression>";
}

static const out_val *builtin_gen_undefined(const expr *e, out_ctx *octx)
{
	(void)e;
	out_ctrl_end_undefined(octx);
	return out_new_noop(octx);
}

expr *parse_any_args(symtable *scope)
{
	expr *fcall = expr_new_funcall();

	fcall->funcargs = parse_funcargs(scope, 0);
	return fcall;
}

/* --- memset */

static void fold_memset(expr *e, symtable *stab)
{
	fold_expr_nodecay(e->lhs, stab);

	if(!expr_is_addressable(e->lhs))
		ICE("can't memset %s - not addressable", e->lhs->f_str());

	if(e->bits.builtin_memset.len == 0)
		cc1_warn_at(&e->where, builtin_memset_bad, "zero size memset");

	if((unsigned)e->bits.builtin_memset.ch > 255)
		cc1_warn_at(&e->where, builtin_memset_bad, "memset with value > UCHAR_MAX");

	e->tree_type = type_ptr_to(type_nav_btype(cc1_type_nav, type_void));
}

static const out_val *builtin_gen_memset(const expr *e, out_ctx *octx)
{
	/* works fine for bitfields - struct lea acts appropriately */
	const out_val *addr = gen_expr(e->lhs, octx);

	out_val_retain(octx, addr);

	out_memset(octx, addr,
			e->bits.builtin_memset.ch,
			e->bits.builtin_memset.len);

	return out_change_type(octx, addr, e->tree_type);
}

expr *builtin_new_memset(expr *p, int ch, size_t len)
{
	expr *fcall = expr_new_funcall();

	fcall->expr = expr_new_identifier("__builtin_memset");

	expr_mutate_builtin(fcall, memset);

	fcall->lhs = p;
	fcall->bits.builtin_memset.ch = ch;
	UCC_ASSERT(ch == 0, "TODO: non-zero memset");
	fcall->bits.builtin_memset.len = len;

	return fcall;
}

#ifdef BUILTIN_LIBC_FUNCTIONS
static expr *parse_memset(const char *ident, symtable *scope)
{
	expr *fcall = parse_any_args(scope);

	ICE("TODO: builtin memset parsing");

	expr_mutate_builtin(fcall, memset);

	return fcall;
}
#endif

/* --- memcpy */

static void fold_memcpy(expr *e, symtable *stab)
{
	e->tree_type = type_ptr_to(type_nav_btype(cc1_type_nav, type_void));

	if(!e->lhs){
		consty k;
		int i;

		/* parsed a user call */
		if(dynarray_count(e->funcargs) != 3){
			warn_at_print_error(&e->where, "%s takes a single argument", BUILTIN_SPEL(e->expr));
			fold_had_error = 1;
			return;
		}

		for(i = 0; i < 3; i++)
			FOLD_EXPR(e->funcargs[i], stab);

		if(!type_is_ptr(e->funcargs[0]->tree_type)
		|| !type_is_ptr(e->funcargs[1]->tree_type)
		|| !type_is_integral(e->funcargs[2]->tree_type))
		{
			warn_at_print_error(&e->where, "TODO: BAD TYPE ERROR for %s %d%d%d",
					BUILTIN_SPEL(e->expr),
					!!type_is_ptr(e->funcargs[0]->tree_type),
					!!type_is_ptr(e->funcargs[1]->tree_type),
					!!type_is_integral(e->funcargs[2]->tree_type));
			fold_had_error = 1;
			return;
		}

		e->lhs = e->funcargs[0];
		e->rhs = e->funcargs[1];

		const_fold(e->funcargs[2], &k);
		if(k.type != CONST_NUM || !K_INTEGRAL(k.bits.num)){
			warn_at_print_error(&e->where, "TODO: BAD CONST NUM ERROR for %s", BUILTIN_SPEL(e->expr));
			fold_had_error = 1;
			return;
		}

		e->bits.num.val.i = k.bits.num.val.i;
	}

	fold_expr_nodecay(e->lhs, stab);
	fold_expr_nodecay(e->rhs, stab);
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

static const out_val *builtin_gen_memcpy(const expr *e, out_ctx *octx)
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
	const out_val *dest, *src;

	dest = gen_expr(e->lhs, octx);
	src = gen_expr(e->rhs, octx);

	return out_change_type(octx,
			out_memcpy(octx, dest, src, e->bits.num.val.i),
			e->tree_type);
#endif
}

expr *builtin_new_memcpy(expr *to, expr *from, size_t len)
{
	expr *fcall = expr_new_funcall();

	fcall->expr = expr_new_identifier("__builtin_memcpy");

	expr_mutate_builtin(fcall, memcpy);

	fcall->lhs = to;
	fcall->rhs = from;
	fcall->bits.num.val.i = len;

	return fcall;
}

#ifdef BUILTIN_LIBC_FUNCTIONS
static expr *parse_memcpy(const char *ident, symtable *scope)
{
	expr *fcall = parse_any_args(scope);

	ICE("TODO: builtin memcpy parsing");

	expr_mutate_builtin(fcall, memcpy);

	return fcall;
}
#endif

/* --- unreachable */

static void fold_unreachable(expr *e, symtable *stab)
{
	type *tvoid = type_nav_btype(cc1_type_nav , type_void);
	attribute **attr = NULL;

	e->expr->tree_type = type_func_of(tvoid,
			funcargs_new(), symtab_new(stab, &e->where));

	dynarray_add(&attr, attribute_new(attr_noreturn));
	e->tree_type = type_attributed(tvoid, attr);
	attribute_array_release(&attr);

	wur_builtin(e);
}

static const out_val *builtin_gen_unreachable(const expr *e, out_ctx *octx)
{
	return builtin_gen_undefined(e, octx);
}

static expr *parse_unreachable(const char *ident, symtable *scope)
{
	expr *fcall = expr_new_funcall();

	(void)ident;
	(void)scope;

	expr_mutate_builtin(fcall, unreachable);

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
	/* allow const int and int to be compatible, or T *const and T* */
	enum type_cmp mask = (TYPE_EQUAL_ANY | TYPE_QUAL_ADD | TYPE_QUAL_SUB);
	enum type_cmp cmp;

	CONST_FOLD_LEAF(k);

	k->type = CONST_NUM;

	cmp = type_cmp(types[0], types[1], 0);

	k->bits.num.val.i = !!(cmp & mask);
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

	e->tree_type = type_ptr_to(type_nav_btype(cc1_type_nav, type_void));

	if(dynarray_count(e->funcargs) != 1){
		warn_at_print_error(&e->where, "%s takes a single argument", BUILTIN_SPEL(e->expr));
		fold_had_error = 1;
		return;
	}

	FOLD_EXPR(e->funcargs[0], stab);

	const_fold(e->funcargs[0], &k);
	if(k.type != CONST_NUM
	|| (K_FLOATING(k.bits.num))
	|| (sintegral_t)k.bits.num.val.i < 0)
	{
		warn_at_print_error(&e->where, "%s needs a positive integral constant value argument", BUILTIN_SPEL(e->expr));
		fold_had_error = 1;
		return;
	}

	if(k.bits.num.val.i > 0)
		cc1_warn_at(&e->where, builtin_frame_addr, "calling '%s' with a non-zero argument is unsafe", BUILTIN_SPEL(e->expr));

	memcpy_safe(&e->bits.num, &k.bits.num);

	wur_builtin(e);
}

static void warn_inlining_frame_ret_addr(out_ctx *octx, const expr *e)
{
	struct cc1_out_ctx *cc1_octx = *cc1_out_ctx(octx);

	if(cc1_octx && cc1_octx->inline_.depth){
		cc1_warn_at(&e->where, inline_builtin_frame_addr,
				"inlining function with call to %s",
				BUILTIN_SPEL(e->expr));
	}
}

static const out_val *builtin_gen_frame_address(const expr *e, out_ctx *octx)
{
	const int depth = e->bits.num.val.i;

	warn_inlining_frame_ret_addr(octx, e);

	return out_new_frame_ptr(octx, depth + 1);
}

static expr *builtin_frame_address_mutate(expr *e)
{
	expr_mutate_builtin(e, frame_address);
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

	dynarray_add(&e->funcargs, expr_compiler_generated(expr_new_val(depth)));

	return builtin_frame_address_mutate(e);
}

/* --- return_address */

static const out_val *builtin_gen_return_address(const expr *e, out_ctx *octx)
{
	const int depth = e->bits.num.val.i;

	warn_inlining_frame_ret_addr(octx, e);

	return out_new_return_addr(octx, depth + 1);
}

static expr *parse_return_address(const char *ident, symtable *scope)
{
	expr *fcall = parse_any_args(scope);

	(void)ident;

	fcall->f_fold = fold_frame_address;
	fcall->f_gen = builtin_gen_return_address;
	fcall->f_str = str_expr_builtin;

	return fcall;
}

/* --- extract_return_addr */

static void fold_extract_return_addr(expr *e, symtable *stab)
{
	type *voidp = type_ptr_to(type_nav_btype(cc1_type_nav, type_void));

	e->tree_type = voidp;

	if(dynarray_count(e->funcargs) != 1){
		warn_at_print_error(&e->where, "%s takes a single argument", BUILTIN_SPEL(e->expr));
		fold_had_error = 1;
		return;
	}

	FOLD_EXPR(e->funcargs[0], stab);

	if(!(type_cmp(e->funcargs[0]->tree_type, voidp, 0) & TYPE_EQUAL_ANY)){
		warn_at_print_error(&e->where, "%s expects a 'void *' argument", BUILTIN_SPEL(e->expr));
		fold_had_error = 1;
		return;
	}

	wur_builtin(e);
}

static const out_val *builtin_gen_extract_return_addr(const expr *e, out_ctx *octx)
{
	/* no translation needed */
	return gen_expr(e->funcargs[0], octx);
}

static expr *parse_extract_return_addr(const char *ident, symtable *scope)
{
	expr *fcall = parse_any_args(scope);

	(void)ident;

	expr_mutate_builtin(fcall, extract_return_addr);

	return fcall;
}

/* --- reg_save_area (a basic wrapper around out_push_reg_save_ptr()) */

static void fold_reg_save_area(expr *e, symtable *stab)
{
	(void)stab;
	e->tree_type = type_ptr_to(type_nav_btype(cc1_type_nav, type_nchar));
}

static const out_val *builtin_gen_reg_save_area(const expr *e, out_ctx *octx)
{
	(void)e;
	out_comment(octx, "stack local offset:");
	return out_new_reg_save_ptr(octx);
}

expr *builtin_new_reg_save_area(void)
{
	expr *e = expr_new_funcall();

	expr_mutate_builtin(e, reg_save_area);

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
		cc1_warn_at(&e->where, builtin_expect_nonconst,
				"%s second argument isn't a constant value",
				BUILTIN_SPEL(e->expr));

	e->tree_type = e->funcargs[0]->tree_type;
	wur_builtin(e);
}

static const out_val *builtin_gen_expect(const expr *e, out_ctx *octx)
{
	const out_val *eval;
	consty k;

	/* not needed if it's const, but gcc and clang do this */
	out_val_consume(octx, gen_expr(e->funcargs[1], octx));

	eval = gen_expr(e->funcargs[0], octx);

	const_fold(e->funcargs[1], &k);
	if(k.type == CONST_NUM)
		eval = out_annotate_likely(octx, eval, !!k.bits.num.val.i);

	return eval;
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

	expr_mutate_builtin(fcall, expect);
	expr_mutate_builtin_const(fcall, expect); /* add const */

	return fcall;
}

/* --- choose_expr */

#define CHOOSE_EXPR_CHOSEN(e) ((e)->funcargs[(e)->bits.num.val.i ? 1 : 2])

static enum lvalue_kind is_lval_choose(const expr *e)
{
	return expr_is_lval(CHOOSE_EXPR_CHOSEN(e));
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
		fold_expr_nodecay(e->funcargs[i], stab);

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

	e->f_islval = is_lval_choose;
}

static void const_choose_expr(expr *e, consty *k)
{
	/* forward to the chosen expr */
	const_fold(CHOOSE_EXPR_CHOSEN(e), k);
}

static const out_val *gen_choose_expr(const expr *e, out_ctx *octx)
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
	fcall->f_gen        = gen_choose_expr;

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

/* --- has_attribute */

static void fold_has_attribute(expr *e, symtable *stab)
{
	if(e->bits.builtin_ident.ty)
		fold_type(e->bits.builtin_ident.ty, stab);
	else
		FOLD_EXPR(e->bits.builtin_ident.expr, stab);

	e->tree_type = type_nav_btype(cc1_type_nav, type__Bool);
	wur_builtin(e);
}

static void const_has_attribute(expr *e, consty *k)
{
	enum attribute_type attr;
	const char *spel = e->bits.builtin_ident.ident;

#define NAME(x, tprop)      else if(!strcmp(spel, #x)) attr = attr_ ## x;
#define ALIAS(s, x, typrop) else if(!strcmp(spel, s))  attr = attr_ ## x;
#define EXTRA_ALIAS(s, x)   else if(!strcmp(spel, s))  attr = attr_ ## x;
	if(0)
		;
	ATTRIBUTES
	else{
		CONST_FOLD_LEAF(k);
		k->type = CONST_NO;
		return;
	}

	CONST_FOLD_LEAF(k);
	k->type = CONST_NUM;

	if(e->bits.builtin_ident.ty)
		k->bits.num.val.i = !!type_attr_present(e->bits.builtin_ident.ty, attr);
	else
		k->bits.num.val.i = !!expr_attr_present(e->bits.builtin_ident.expr, attr);
}

static expr *parse_has_attribute(const char *ident, symtable *scope)
{
	expr *fcall = expr_new_funcall();

	(void)ident;

	fcall->bits.builtin_ident.ty = parse_type(1, scope);
	if(!fcall->bits.builtin_ident.ty)
		fcall->bits.builtin_ident.expr = PARSE_EXPR_NO_COMMA(scope, /*static_ctx*/1);

	EAT(token_comma);

	fcall->bits.builtin_ident.ident = curtok_to_identifier(&fcall->bits.builtin_ident.alloc);

	EAT(token_identifier);

	expr_mutate_builtin_const(fcall, has_attribute);
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

static void const_nan(expr *e, consty *k)
{
	CONST_FOLD_LEAF(k);
	k->type = CONST_NUM;
	/* use native nan for calculations, switch to host-nan when writing out */
	k->bits.num.val.f = NAN;

	switch(e->bits.builtin_nantype){
		case builtin_nanf: k->bits.num.suffix = VAL_FLOAT; break;
		case builtin_nan:  k->bits.num.suffix = VAL_DOUBLE; break;
		case builtin_nanl: k->bits.num.suffix = VAL_LDOUBLE; break;
	}
}

static const out_val *builtin_gen_nan(const expr *e, out_ctx *octx)
{
	return out_new_nan(octx, e->tree_type);
}

static expr *builtin_nan_mutate(expr *e)
{
	expr_mutate_builtin(e, nan);
	e->f_const_fold = const_nan;
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
	CONST_FOLD_NO(k, e);

	/* if 1 arg and it has a char * constant, return length */
	if(dynarray_count(e->funcargs) == 1){
		expr *s = e->funcargs[0];
		consty subk;

		const_fold(s, &subk);
		if(subk.type == CONST_STRK){
			stringlit *lit = subk.bits.str->lit;

			switch(lit->cstr->type){
				case CSTRING_RAW:
				case CSTRING_ASCII:
				{
					const char *s = lit->cstr->bits.ascii;
					const char *p = memchr(s, '\0', lit->cstr->count);

					if(p){
						CONST_FOLD_LEAF(k);
						k->type = CONST_NUM;
						k->bits.num.val.i = p - s;
						k->bits.num.suffix = VAL_UNSIGNED;
					}
					break;
				}
				case CSTRING_WIDE:
					break;
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

/* --- offsetof */

static void fold_offsetof(expr *e, symtable *stab)
{
	if(!type_is_complete(e->bits.offsetof_ty)){
		warn_at_print_error(&e->where, "offsetof() on incomplete type '%s'",
				type_to_str(e->bits.offsetof_ty));

		fold_had_error = 1;
	}

	/* fold regardless, prevent const_fold() ICEs */
	fold_expr_nodecay(e->lhs, stab);

	if(expr_is_struct_bitfield(e->lhs)){
		warn_at_print_error(&e->lhs->where, "offsetof() into bitfield");
		fold_had_error = 1;
	}

	e->tree_type = type_nav_btype(cc1_type_nav, type_ulong);
	wur_builtin(e);
}

static const out_val *builtin_gen_offsetof(const expr *e, out_ctx *octx)
{
	return out_change_type(octx, gen_expr(e->lhs, octx), e->tree_type);
}

static void const_offsetof(expr *e, consty *k)
{
	consty offset;
	const_fold(e->lhs, &offset);

	switch(offset.type){
		case CONST_ADDR:
			/* allow, if array */
			if(!type_is_array(e->lhs->tree_type))
				break;
			/* fall */

		case CONST_NEED_ADDR:
			if(offset.bits.addr.is_lbl)
				break;

			CONST_FOLD_LEAF(k);

			k->type = CONST_NUM;
			k->bits.num.val.i = offset.bits.addr.bits.memaddr + offset.offset;
			break;

		default:
			break;
	}
}

static expr *parse_offsetof(const char *ident, symtable *scope)
{
	expr *fcall = expr_new_funcall();

	(void)ident;

	fcall->bits.offsetof_ty = parse_type(0, scope);

	if(!fcall->bits.offsetof_ty){
		warn_at_print_error(NULL, "type expected for offsetof()");
		parse_had_error = 1;
		return fcall;
	}

	EAT(token_comma);

	fold_type(fcall->bits.offsetof_ty, scope);
	/* ((struct ... *)0)->.... */
	fcall->lhs = expr_new_struct(
			expr_new_cast(
				expr_new_val(0),
				type_ptr_to(fcall->bits.offsetof_ty),
				1 /* implicit */),
			0 /* dot */,
			parse_expr_identifier());

	if(curtok != token_close_paren)
		cc1_warn_at(NULL, offsetof_extended, "extended designator in offsetof()");

	while(curtok != token_close_paren){
		if(accept(token_dot)){
			fcall->lhs = expr_new_struct(
					fcall->lhs, 1, parse_expr_identifier());

		}else if(accept(token_open_square)){
			fcall->lhs = expr_new_array_idx_e(
					fcall->lhs, parse_expr_exp(scope, 0));

			EAT(token_close_square);
		}else{
			warn_at_print_error(NULL, "\".\" or \"[\" expected");
			parse_had_error = 1;
			break;
		}
	}

	expr_mutate_builtin_const(fcall, offsetof);
	fcall->f_gen = builtin_gen_offsetof;

	return fcall;
}

/* --- {add,sub,mul}_overflow */

static void fold_arith_overflow(expr *e, symtable *stab)
{
	const char *name = BUILTIN_SPEL(e->expr);
	int i;
	type *last;
	expr *last_e;

	e->tree_type = type_nav_btype(cc1_type_nav, type__Bool);
	wur_builtin(e);

	if(dynarray_count(e->funcargs) != 3){
		warn_at_print_error(&e->where, "%s takes a three arguments", name);
		fold_had_error = 1;
		return;
	}

	for(i = 0; i < 3; i++)
		FOLD_EXPR(e->funcargs[i], stab);

	for(i = 0; i < 2; i++)
		if(fold_check_expr(e->funcargs[i], FOLD_CHK_INTEGRAL, name))
			return;

	last_e = e->funcargs[2];
	last = type_is_ptr(last_e->tree_type);
	if(!last || !type_is_integral(last)){
		warn_at_print_error(&e->where, "%s's third argument isn't pointer-to-integral-type", name);
		fold_had_error = 1;
		return;
	}

	if(type_qual(last) & qual_const){
		warn_at_print_error(&e->where, "%s's third argument is const-qualified (%s)", name, type_to_str(last));
		fold_had_error = 1;
		return;
	}
	if(type_is_enum(last)){
		warn_at_print_error(&e->where, "%s's third argument is an enum (%s)", name, type_to_str(last));
		fold_had_error = 1;
		return;
	}
	if(type_is_primitive(last, type__Bool)){
		warn_at_print_error(&e->where, "%s's third argument is a boolean", name, type_to_str(last));
		fold_had_error = 1;
		return;
	}

	if(expr_kind(last_e, addr)){
		expr *lval = expr_addr_target(last_e);
		fold_inc_writes_if_sym(lval, stab);
	}
}

static type *arith_overflow_largest_type(const expr *e)
{
	type *t = type_is_ptr(e->funcargs[2]->tree_type);
	unsigned sz = type_size(t, NULL);
	int i;

	for(i = 0; i < 2; i++){
		type *t2 = e->funcargs[i]->tree_type;
		unsigned sz2 = type_size(t2, NULL);

		if(sz2 > sz){
			t = t2;
			sz = sz2;
		}
	}

	return t;
}

static const out_val *gen_arith_overflow(const expr *e, out_ctx *octx)
{
	const out_val *lhs, *rhs, *result, *result_trunc;
	const out_val *of;
	const out_val *store;
	type *largest = arith_overflow_largest_type(e);
	type *storety = type_is_ptr(e->funcargs[2]->tree_type);
	int smaller_than_int = type_size(storety, NULL) < type_primitive_size(type_int);

	lhs = gen_expr(e->funcargs[0], octx);
	rhs = gen_expr(e->funcargs[1], octx);

	lhs = out_cast(octx, lhs, largest, 0);
	rhs = out_cast(octx, rhs, largest, 0);

	result = out_op(octx, e->bits.op.op, lhs, rhs);

	of = out_new_overflow(octx, &result);

	store = gen_expr(e->funcargs[2], octx);
	if(smaller_than_int)
		out_val_retain(octx, result);

	result_trunc = out_cast(octx, result, storety, 0);
	out_store(octx, store, result_trunc);

	if(smaller_than_int){
		type *intty = type_nav_btype(cc1_type_nav, type_int);
		const out_val *extended, *shortened;
		const out_val *hasdiff;

		out_val_retain(octx, result);
		extended = out_cast(octx, result, intty, 0);
		shortened = out_cast(octx, extended, storety, 0);
		hasdiff = out_op(octx, op_ne, shortened, result);

		of = out_op(octx, op_or, of, hasdiff);
	}

	return of;
}

static expr *parse_arith_overflow(const char *ident, symtable *scope, enum op_type op)
{
	expr *fcall = parse_any_args(scope);

	(void)ident;

	fcall->bits.op.op = op;

	fcall->f_fold = fold_arith_overflow;
	fcall->f_gen = gen_arith_overflow;

	return fcall;
}

static expr *parse_add_overflow(const char *ident, symtable *scope)
{
	return parse_arith_overflow(ident, scope, op_plus);
}

static expr *parse_sub_overflow(const char *ident, symtable *scope)
{
	return parse_arith_overflow(ident, scope, op_minus);
}

static expr *parse_mul_overflow(const char *ident, symtable *scope)
{
	return parse_arith_overflow(ident, scope, op_multiply);
}
