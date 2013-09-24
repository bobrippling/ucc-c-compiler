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
#include "../gen_str.h"

#include "../out/out.h"
#include "../out/basic_block/bb.h"

#include "__builtin_va.h"

#define PREFIX "__builtin_"

typedef expr *func_builtin_parse(void);

static func_builtin_parse parse_unreachable,
                          parse_compatible_p,
                          parse_constant_p,
                          parse_frame_address,
                          parse_expect,
                          parse_strlen,
                          parse_is_signed,
                          parse_nanf;
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

	{ "nanf", parse_nanf },

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

basic_blk *builtin_gen_print(expr *e, basic_blk *b_from)
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

	return b_from;
}

#define expr_mutate_builtin_const(exp, to) \
	expr_mutate_builtin(exp, to),             \
	exp->f_const_fold = const_ ## to

static void wur_builtin(expr *e)
{
	e->freestanding = 0; /* needs use */
}

static basic_blk *builtin_gen_undefined(expr *e, basic_blk *b_from)
{
	(void)e;
	out_undefined(b_from);
	out_push_noop(b_from); /* needed for function return pop */

	return b_from;
}

expr *parse_any_args(void)
{
	expr *fcall = expr_new_funcall();
	fcall->funcargs = parse_funcargs();
	return fcall;
}

/* --- memset */

static void fold_memset(expr *e, symtable *stab)
{
	fold_expr_no_decay(e->lhs, stab);

	if(!expr_is_addressable(e->lhs)){
		/* this is pretty much an ICE, except it may be
		 * user-callable in the future
		 */
		die_at(&e->where, "can't memset %s - not addressable",
				e->lhs->f_str());
	}

	if(e->bits.builtin_memset.len == 0)
		warn_at(&e->where, "zero size memset");

	if((unsigned)e->bits.builtin_memset.ch > 255)
		warn_at(&e->where, "memset with value > UCHAR_MAX");

	e->tree_type = type_ref_cached_VOID_PTR();
}

static basic_blk *builtin_gen_memset(expr *e, basic_blk *b_from)
{
	size_t n, rem;
	unsigned i;
	type_ref *tzero = type_ref_cached_MAX_FOR(e->bits.builtin_memset.len);
	type_ref *textra, *textrap;

	if(!tzero)
		tzero = type_ref_cached_CHAR();

	n   = e->bits.builtin_memset.len / type_ref_size(tzero, NULL);
	rem = e->bits.builtin_memset.len % type_ref_size(tzero, NULL);

	if((textra = rem ? type_ref_cached_MAX_FOR(rem) : NULL))
		textrap = type_ref_new_ptr(textra, qual_none);

	/* works fine for bitfields - struct lea acts appropriately */
	lea_expr(e->lhs, b_from);

	out_change_type(b_from, type_ref_new_ptr(tzero, qual_none));

	out_dup(b_from);

#ifdef MEMSET_VERBOSE
	out_comment(b_from, "memset(%s, %d, %lu), using ptr<%s>, %lu steps",
			e->expr->f_str(),
			e->bits.builtin_memset.ch,
			e->bits.builtin_memset.len,
			type_ref_to_str(tzero), n);
#endif

	for(i = 0; i < n; i++){
		out_dup(b_from); /* copy pointer */

		/* *p = 0 */
		out_push_zero(b_from, tzero);
		out_store(b_from);
		out_pop(b_from);

		/* p++ (copied pointer) */
		out_push_l(b_from, type_ref_cached_INTPTR_T(), 1);
		out_op(b_from, op_plus);

		if(rem){
			/* need to zero a little more */
			out_dup(b_from);
			out_change_type(b_from, textrap);
			out_push_zero(b_from, textra);
			out_store(b_from);
			out_pop(b_from);
		}
	}

	out_pop(b_from);

	return b_from;
}

expr *builtin_new_memset(expr *p, int ch, size_t len)
{
	expr *fcall = expr_new_funcall();

	fcall->expr = expr_new_identifier("__builtin_memset");

	expr_mutate_builtin_gen(fcall, memset);

	fcall->lhs = p;
	fcall->bits.builtin_memset.ch = ch;
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

	e->tree_type = type_ref_cached_VOID_PTR();
}

#ifdef BUILTIN_USE_LIBC
static decl *decl_new_tref(char *sp, type_ref *ref)
{
	decl *d = decl_new();
	d->ref = ref;
	d->spel = sp;
	return d;
}
#endif

static void builtin_memcpy_single(basic_blk *b_from)
{
	static type_ref *t1;

	if(!t1)
		t1 = type_ref_cached_INTPTR_T();

	/* ds */

	out_swap(b_from); // sd
	out_dup(b_from);  // sdd
	out_pulltop(b_from, 2); // dds

	out_dup(b_from);      /* ddss */
	out_deref(b_from);    /* dds. */
	out_pulltop(b_from, 2); /* ds.d */
	out_swap(b_from);     /* dsd. */
	out_store(b_from);    /* ds. */
	out_pop(b_from);      /* ds */

	out_push_l(b_from, t1, 1); /* ds1 */
	out_op(b_from, op_plus);   /* dS */

	out_swap(b_from);        /* Sd */
	out_push_l(b_from, t1, 1); /* Sd1 */
	out_op(b_from, op_plus);   /* SD */

	out_swap(b_from); /* DS */
}

static basic_blk *builtin_gen_memcpy(expr *e, basic_blk *b_from)
{
#ifdef BUILTIN_USE_LIBC
	/* TODO - also with memset */
	funcargs *fargs = funcargs_new();

	dynarray_add(&fargs->arglist, decl_new_tref(NULL, type_ref_cached_VOID_PTR()));
	dynarray_add(&fargs->arglist, decl_new_tref(NULL, type_ref_cached_VOID_PTR()));
	dynarray_add(&fargs->arglist, decl_new_tref(NULL, type_ref_cached_INTPTR_T()));

	type_ref *ctype = type_ref_new_func(
			e->tree_type, fargs);

	out_push_lbl(b_from, "memcpy", 0);
	out_push_l(b_from, type_ref_cached_INTPTR_T(), e->bits.num.val);
	lea_expr(e->rhs, stab);
	lea_expr(e->lhs, stab);
	out_call(b_from, 3, e->tree_type, ctype);
#else
	/* TODO: backend rep movsb */
	unsigned i = e->bits.num.val.i;
	type_ref *tptr = type_ref_new_ptr(
				type_ref_cached_MAX_FOR(e->bits.num.val.i),
				qual_none);
	unsigned tptr_sz = type_ref_size(tptr, &e->where);

	lea_expr(e->lhs, b_from); /* d */
	lea_expr(e->rhs, b_from); /* ds */

	while(i > 0){
		/* as many copies as we can */
		out_change_type(b_from, tptr);
		out_swap(b_from);
		out_change_type(b_from, tptr);
		out_swap(b_from);

		while(i >= tptr_sz){
			i -= tptr_sz;
			builtin_memcpy_single(b_from);
		}

		if(i > 0){
			tptr_sz /= 2;
			tptr = type_ref_new_ptr(
					type_ref_cached_MAX_FOR(tptr_sz),
					qual_none);
		}
	}

	/* ds */
	out_pop(b_from); /* d */
#endif

	return b_from;
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
	(void)stab;

	e->tree_type = type_ref_func_call(
			e->expr->tree_type = type_ref_new_func(
				type_ref_cached_VOID(), funcargs_new()), NULL);

	decl_attr_append(&e->tree_type->attr, decl_attr_new(attr_noreturn));

	wur_builtin(e);
}

static expr *parse_unreachable(void)
{
	expr *fcall = expr_new_funcall();

	expr_mutate_builtin(fcall, unreachable);

	BUILTIN_SET_GEN(fcall, builtin_gen_undefined);

	return fcall;
}

/* --- compatible_p */

static void fold_compatible_p(expr *e, symtable *stab)
{
	type_ref **types = e->bits.types;

	if(dynarray_count(types) != 2)
		die_at(&e->where, "need two arguments for %s", BUILTIN_SPEL(e->expr));

	fold_type_ref(types[0], NULL, stab);
	fold_type_ref(types[1], NULL, stab);

	e->tree_type = type_ref_cached_BOOL();
	wur_builtin(e);
}

static void const_compatible_p(expr *e, consty *k)
{
	type_ref **types = e->bits.types;

	memset(k, 0, sizeof *k);
	k->type = CONST_NUM;
	k->bits.num.val.i = type_ref_cmp(types[0], types[1], 0) == TYPE_EQUAL;
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
	if(dynarray_count(e->funcargs) != 1)
		die_at(&e->where, "%s takes a single argument", BUILTIN_SPEL(e->expr));

	FOLD_EXPR(e->funcargs[0], stab);

	e->tree_type = type_ref_cached_BOOL();
	wur_builtin(e);
}

static void const_constant_p(expr *e, consty *k)
{
	expr *test = *e->funcargs;
	consty subk;

	const_fold(test, &subk);

	memset(k, 0, sizeof *k);
	k->type = CONST_NUM;
	k->bits.num.val.i = CONST_AT_COMPILE_TIME(subk.type);
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

	e->tree_type = type_ref_new_ptr(
			type_ref_new_type(
				type_new_primitive(type_char)
			),
			qual_none);

	wur_builtin(e);
}

static basic_blk *builtin_gen_frame_address(expr *e, basic_blk *b_from)
{
	const int depth = e->bits.num.val.i;

	out_push_frame_ptr(b_from, depth + 1);

	return b_from;
}

static expr *builtin_frame_address_mutate(expr *e)
{
	expr_mutate_builtin(e, frame_address);
	BUILTIN_SET_GEN(e, builtin_gen_frame_address);
	return e;
}

static expr *parse_frame_address(void)
{
	return builtin_frame_address_mutate(parse_any_args());
}

expr *builtin_new_frame_address(int depth)
{
	expr *e = expr_new_funcall();

	dynarray_add(&e->funcargs, expr_new_val(depth));

	return builtin_frame_address_mutate(e);
}

/* --- reg_save_area (a basic wrapper around out_push_reg_save_ptr(b_from)) */

static void fold_reg_save_area(expr *e, symtable *stab)
{
	(void)stab;
	e->tree_type = type_ref_cached_CHAR_PTR();
}

static basic_blk *gen_reg_save_area(expr *e, basic_blk *b_from)
{
	(void)e;
	out_comment(b_from, "stack local offset:");
	out_push_reg_save_ptr(b_from);

	return b_from;
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

static basic_blk *builtin_gen_expect(expr *e, basic_blk *b_from)
{
	gen_expr(e->funcargs[1], b_from); /* not needed if it's const, but gcc and clang do this */
	out_pop(b_from);
	gen_expr(e->funcargs[0], b_from);
	return b_from;
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
	BUILTIN_SET_GEN(fcall, builtin_gen_expect);
	return fcall;
}

/* --- is_signed */

static void fold_is_signed(expr *e, symtable *stab)
{
	type_ref **tl = e->bits.types;

	if(dynarray_count(tl) != 1)
		die_at(&e->where, "need a single argument for %s", BUILTIN_SPEL(e->expr));

	fold_type_ref(tl[0], NULL, stab);

	e->tree_type = type_ref_cached_BOOL();
	wur_builtin(e);
}

static void const_is_signed(expr *e, consty *k)
{
	memset(k, 0, sizeof *k);
	k->type = CONST_NUM;
	k->bits.num.val.i = type_ref_is_signed(e->bits.types[0]);
}

static expr *parse_is_signed(void)
{
	expr *fcall = expr_new_funcall_typelist();

	/* simply set the const vtable ent */
	fcall->f_fold       = fold_is_signed;
	fcall->f_const_fold = const_is_signed;

	return fcall;
}

/* --- nanf */

static void fold_nanf(expr *e, symtable *stab)
{
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

	if(k.bits.str->len > 1)
		die_at(&e->where, "%s only implemented for nanf(\"\")",
				BUILTIN_SPEL(e->expr));

	e->tree_type = type_ref_cached_DOUBLE();
	wur_builtin(e);
}

static basic_blk *builtin_gen_nanf(expr *e, basic_blk *b_from)
{
	out_push_nan(b_from, e->tree_type);
	return b_from;
}

static expr *builtin_nanf_mutate(expr *e)
{
	expr_mutate_builtin(e, nanf);
	BUILTIN_SET_GEN(e, builtin_gen_nanf);
	return e;
}

static expr *parse_nanf(void)
{
	return builtin_nanf_mutate(parse_any_args());
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
			stringval *sv = subk.bits.str;
			const char *s = sv->str;
			const char *p = memchr(s, '\0', sv->len);

			if(p){
				k->type = CONST_NUM;
				k->bits.num.val.i = p - s;
				k->bits.num.suffix = VAL_UNSIGNED;
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
