#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>

#include "../util/util.h"
#include "../util/alloc.h"
#include "../util/dynarray.h"

#include "parse_expr.h"
#include "parse_stmt.h"
#include "cc1_where.h"
#include "warn.h"

#include "tokenise.h"
#include "tokconv.h"
#include "str.h"

#include "parse_type.h"
#include "parse_init.h"
#include "ops/__builtin.h" /* builtin_parse() */

#include "funcargs.h"
#include "type_is.h"

#include "ops/expr__Generic.h"
#include "ops/expr_addr.h"
#include "ops/expr_assign.h"
#include "ops/expr_assign_compound.h"
#include "ops/expr_block.h"
#include "ops/expr_cast.h"
#include "ops/expr_comma.h"
#include "ops/expr_compound_lit.h"
#include "ops/expr_deref.h"
#include "ops/expr_funcall.h"
#include "ops/expr_identifier.h"
#include "ops/expr_op.h"
#include "ops/expr_sizeof.h"
#include "ops/expr_stmt.h"
#include "ops/expr_string.h"
#include "ops/expr_struct.h"
#include "ops/expr_val.h"
#include "ops/expr_if.h"

expr *parse_expr_unary(symtable *scope, int static_ctx);
#define PARSE_EXPR_CAST(s, static_ctx) parse_expr_unary(s, static_ctx)

static expr *parse_expr_postfix_with(symtable *, int static_ctx, expr *);

expr *parse_expr_sizeof_typeof_alignof(symtable *scope)
{
	const int static_ctx = /*doesn't matter:*/0;
	expr *e;
	where w;
	int is_expr = 1;
	enum what_of what_of;

	where_cc1_current(&w);

	switch(curtok){
		default:
			assert(0 && "unreachable sizeof parse");

		case token__Alignof:
			what_of = what_alignof;
			if(0)
		case token_typeof:
			what_of = what_typeof;
			if(0)
		case token_sizeof:
			what_of = what_sizeof;
	}
	EAT(curtok);

	if(accept(token_open_paren)){
		type *ty = parse_type(0, scope);

		if(ty){
			EAT(token_close_paren);

			/* check for sizeof(int){...}.x[0]->q... */

			if(curtok == token_open_block){
				decl_init *complit_init = parse_init(scope, static_ctx);
				/* got the { 1, 2, ... } */

				expr *complit = expr_new_compound_lit(ty, complit_init, static_ctx);
				/* got the (int){ 1, 2, ... } */

				expr *entire_sizeof_primary = parse_expr_postfix_with(
						scope, static_ctx, complit);
				/* got the (int){ 1, 2, ... }.x[0]->q... */

				e = expr_new_sizeof_expr(entire_sizeof_primary, what_of);
				/* got the sizeof(int){...}.x[0]->q... */

			}else{
				e = expr_new_sizeof_type(ty, what_of);
				is_expr = 0;
			}

		}else{
			/* not a type - treat the open paren as part of the expression */
			uneat(token_open_paren);
			e = expr_new_sizeof_expr(
					parse_expr_unary(scope, static_ctx),
					what_of);
		}

	}else{
		if(what_of == what_typeof)
			/* TODO? cc1_error = 1, return expr_new_val(0) */
			die_at(NULL, "open paren expected after typeof");

		e = expr_new_sizeof_expr(parse_expr_unary(scope, static_ctx), what_of);
		/* don't go any higher, sizeof a - 1, means sizeof(a) - 1 */
	}

	e = expr_set_where_len(e, &w);

	if(what_of == what_alignof && is_expr){
		cc1_warn_at(&e->where, gnu_alignof_expr,
				"_Alignof applied to expression is a GNU extension");
	}

	return e;
}

static expr *parse_expr__Generic(symtable *scope, int static_ctx)
{
	struct generic_lbl **lbls;
	expr *test;
	where w;

	where_cc1_current(&w);

	EAT(token__Generic);
	EAT(token_open_paren);

	test = PARSE_EXPR_NO_COMMA(scope, static_ctx);
	lbls = NULL;

	for(;;){
		type *r;
		expr *e;
		struct generic_lbl *lbl;

		EAT(token_comma);

		if(accept(token_default)){
			r = NULL;
		}else{
			r = parse_type(0, scope);
			if(!r)
				die_at(NULL,
						"type expected for _Generic (got %s)",
						token_to_str(curtok));
		}
		EAT(token_colon);
		e = PARSE_EXPR_NO_COMMA(scope, static_ctx);

		lbl = umalloc(sizeof *lbl);
		lbl->e = e;
		lbl->t = r;
		dynarray_add(&lbls, lbl);

		if(accept(token_close_paren))
			break;
	}

	return expr_set_where(
			expr_new__Generic(test, lbls), &w);
}

expr *parse_expr_identifier(void)
{
	expr *e;
	char *sp;

	if(curtok != token_identifier)
		die_at(NULL, "identifier expected, got %s", token_to_str(curtok));

	sp = token_current_spel();

	e = expr_new_identifier(sp);
	where_cc1_adj_identifier(&e->where, sp);
	EAT(token_identifier);
	return e;
}

static expr *parse_block(symtable *const scope)
{
	symtable *arg_symtab;
	/* not created yet - only create arg_symtab if we need to
	 * argument symtables are exactly one nested level from
	 * global scope, so we don't create one here and then add
	 * another on later */

	funcargs *args;
	type *rt;
	expr *blk;

	EAT(token_xor);

	rt = parse_type(0, scope);

	if(rt){
		if(type_is(rt, type_func)){
			/* got ^int (args...) */
			type *func = type_is(rt, type_func);

			rt = type_func_call(rt, &args);

			arg_symtab = func->bits.func.arg_scope;

		}else{
			/* ^int {...} */
			goto def_args;
		}

	}else if(accept(token_open_paren)){
		/* ^(args...) */
		arg_symtab = symtab_new(
				symtab_root(scope),
				where_cc1_current(NULL));

		arg_symtab->are_params = 1;
		args = parse_func_arglist(arg_symtab);

		EAT(token_close_paren);
	}else{
		/* ^{...} */
def_args:
		args = funcargs_new();
		args->args_void = 1;

		arg_symtab = symtab_new(
				symtab_root(scope),
				where_cc1_current(NULL));
	}

	blk = expr_new_block(rt, args);
	expr_block_got_params(blk, arg_symtab, args);

	{
		stmt *code = parse_stmt_block(arg_symtab, NULL);

		expr_block_got_code(blk, code);

		return blk;
	}
}

struct cstring *parse_asciz_str(void)
{
	struct cstring *cstr = token_get_current_str(NULL);
	char *nul;

	if(cstr->type == CSTRING_WIDE){
		warn_at_print_error(NULL, "wide string not wanted");
		parse_had_error = 1;
		return NULL;
	}

	nul = memchr(cstr->bits.ascii, '\0', cstr->count);
	if(nul && nul < cstr->bits.ascii + cstr->count - 1){
		cc1_warn_at(NULL, str_contain_nul, "nul-character terminates string early");
	}

	return cstr;
}

static expr *parse_expr_primary(symtable *scope, int static_ctx)
{
	switch(curtok){
		case token_integer:
		case token_character:
		case token_floater:
		{
			expr *e = expr_new_numeric(&currentval);
			EAT(curtok);
			return e;
		}

		case token_string:
		{
			where w;
			struct cstring *str = token_get_current_str(&w);

			EAT(token_string);

			return expr_new_str(str, &w, scope);
		}

		case token__Generic:
			return parse_expr__Generic(scope, static_ctx);

		case token_xor:
			return parse_block(scope);

		default:
		{
			where loc_start;

			if(accept_where(token_open_paren, &loc_start)){
				type *r;
				expr *e;

				if((r = parse_type(0, scope))){
					EAT(token_close_paren);

					if(curtok == token_open_block){
						/* C99 compound lit. */
						e = expr_new_compound_lit(
								r, parse_init(scope, static_ctx),
								static_ctx);

					}else{
						/* another cast */
						e = expr_new_cast(
								PARSE_EXPR_CAST(scope, static_ctx), r, 0);
					}

					return expr_set_where_len(e, &loc_start);

				}else if(curtok == token_open_block){
					/* ({ ... }) */
					cc1_warn_at(NULL, gnu_expr_stmt, "use of GNU expression-statement");

					e = expr_new_stmt(parse_stmt_block(scope, NULL));

				}else{
					/* mark as being inside parens, for if((x = 5)) checking */
					e = parse_expr_exp(scope, static_ctx);
					e->in_parens = 1;
				}

				EAT(token_close_paren);
				return e;
			}else{
				/* inline asm */
				if(accept(token_asm)){
					ICW("token_asm - redirect to builtin instead of identifier");
					return expr_new_identifier(ustrdup(ASM_INLINE_FNAME));
				}

				if(curtok != token_identifier){
					/* TODO? cc1_error = 1, return expr_new_val(0) */
					die_at(NULL, "expression expected, got %s", token_to_str(curtok));
				}

				return parse_expr_identifier();
			}
			/* unreachable */
		}
	}
}

static expr *parse_expr_postfix_with(
		symtable *scope, int static_ctx, expr *e)
{
	int flag;

	for(;;){
		where w;

		if(accept(token_open_square)){
			expr *sum = expr_new_op(op_plus);

			sum->lhs  = e;
			sum->rhs  = parse_expr_exp(scope, static_ctx);
			sum->bits.op.array_notation = 1;

			EAT(token_close_square);

			e = expr_new_deref(sum);

		}else if(accept_where(token_open_paren, &w)){
			expr *fcall = NULL;
			const char *sp;

			/* check for specialised builtin parsing */
			if(expr_kind(e, identifier) && (sp = expr_ident_spel(e)))
				fcall = builtin_parse(sp, scope);

			if(!fcall){
				fcall = expr_new_funcall();
				fcall->funcargs = parse_funcargs(scope, static_ctx);
			}

			fcall->expr = e;
			EAT(token_close_paren);

			e = expr_set_where(fcall, &w);

		}else if((flag = accept(token_dot)) || accept(token_ptr)){
			where_cc1_current(&w);

			e = expr_set_where(
					expr_new_struct(e, flag, parse_expr_identifier()),
					&w);

		}else if((flag = accept_where(token_increment, &w))
		||               accept_where(token_decrement, &w))
		{
			e = expr_set_where(
					expr_new_assign_compound(
						e,
						expr_new_val(1),
						flag ? op_plus : op_minus),
					&w);

			e->assign_is_post = 1;

		}else{
			break;
		}
	}

	return e;
}

static expr *parse_expr_postfix(symtable *scope, int static_ctx)
{
	return parse_expr_postfix_with(
			scope, static_ctx, parse_expr_primary(scope, static_ctx));
}

expr *parse_expr_unary(symtable *scope, int static_ctx)
{
	int set_w = 1;
	expr *e;
	int flag;
	where w;

	/* save since we descend before creating the assignment */
	where_cc1_current(&w);

	if((flag = accept(token_increment))
	||         accept(token_decrement))
	{
		/* this is a normal increment, i.e. ++x, simply translate it to x += 1 */
		e = expr_new_assign_compound(
					parse_expr_unary(scope, static_ctx), /* lval */
					expr_new_val(1),
					flag ? op_plus : op_minus);

	}else{
		switch(curtok){
			case token_andsc:
				/* GNU &&label */
				cc1_warn_at(NULL, gnu_addr_lbl, "use of GNU address-of-label");
				EAT(curtok);
				e = expr_new_addr_lbl(token_current_spel(), static_ctx);
				EAT(token_identifier);
				break;

			case token_and:
				EAT(token_and);
				e = expr_new_addr(PARSE_EXPR_CAST(scope, static_ctx));
				break;

			case token_multiply:
				EAT(curtok);
				e = expr_new_deref(PARSE_EXPR_CAST(scope, static_ctx));
				break;

			case token_plus:
			case token_minus:
			case token_bnot:
			case token_not:
				e = expr_new_op(curtok_to_op());
				EAT(curtok);
				e->lhs = PARSE_EXPR_CAST(scope, static_ctx);
				break;

			case token_sizeof:
				set_w = 0; /* no need since there's no sub-parsing here */
				e = parse_expr_sizeof_typeof_alignof(scope);
				break;

			case token__Alignof:
				set_w = 0;
				e = parse_expr_sizeof_typeof_alignof(scope);
				break;

			case token___extension__:
				EAT(curtok);
				return parse_expr_unary(scope, static_ctx);

			default:
				set_w = 0;
				e = parse_expr_postfix(scope, static_ctx);
		}
	}

	if(set_w)
		expr_set_where(e, &w);

	return e;
}

static expr *parse_expr_generic(
		expr *(*above)(symtable *, int static_ctx),
		symtable *scope,
		int static_ctx,
		enum token t, ...)
{
	expr *e = above(scope, static_ctx);

	for(;;){
		int have = curtok == t;
		where w_op;
		expr *join;

		if(!have){
			va_list l; va_start(l, t);
			have = curtok_in_list(l);
			va_end(l);
		}

		if(!have)
			break;

		where_cc1_current(&w_op);
		join = expr_set_where(
				expr_new_op(curtok_to_op()),
				&w_op);

		EAT(curtok);
		join->lhs = e;
		join->rhs = above(scope, static_ctx);

		e = join;
	}

	return e;
}

#define PARSE_DEFINE(this, above, ...) \
static expr *parse_expr_ ## this(      \
		symtable *scope, int static_ctx)   \
{                                      \
	return parse_expr_generic(           \
			parse_expr_ ## above, scope,     \
			static_ctx,                      \
			__VA_ARGS__, token_unknown);     \
}

/* cast is handled in unary instead */
PARSE_DEFINE(multi,     /*cast,*/unary,  token_multiply, token_divide, token_modulus)
PARSE_DEFINE(additive,    multi,         token_plus, token_minus)
PARSE_DEFINE(shift,       additive,      token_shiftl, token_shiftr)
PARSE_DEFINE(relational,  shift,         token_lt, token_gt, token_le, token_ge)
PARSE_DEFINE(equality,    relational,    token_eq, token_ne)
PARSE_DEFINE(binary_and,  equality,      token_and)
PARSE_DEFINE(binary_xor,  binary_and,    token_xor)
PARSE_DEFINE(binary_or,   binary_xor,    token_or)
PARSE_DEFINE(logical_and, binary_or,     token_andsc)
PARSE_DEFINE(logical_or,  logical_and,   token_orsc)

static expr *parse_expr_conditional(symtable *scope, int static_ctx)
{
	expr *e = parse_expr_logical_or(scope, static_ctx);
	where w;

	if(accept_where(token_question, &w)){
		expr *q = expr_set_where(expr_new_if(e), &w);

		if(accept(token_colon)){
			q->lhs = NULL; /* sentinel */
		}else{
			q->lhs = parse_expr_exp(scope, static_ctx);
			EAT(token_colon);
		}
		q->rhs = parse_expr_conditional(scope, static_ctx);

		return q;
	}
	return e;
}

expr *parse_expr_assignment(symtable *scope, int static_ctx)
{
	expr *e;
	where w;

	e = parse_expr_conditional(scope, static_ctx);

	if(accept_where(token_assign, &w)){
		return expr_set_where(
				expr_new_assign(e, parse_expr_assignment(scope, static_ctx)),
				&w);

	}else if(curtok_is_compound_assignment()){
		/* +=, ... - only evaluate the lhs once*/
		enum op_type op = curtok_to_compound_op();

		where_cc1_current(&w);
		EAT(curtok);

		return expr_set_where(
				expr_new_assign_compound(e,
					parse_expr_assignment(scope, static_ctx),
					op),
				&w);
	}

	return e;
}

expr *parse_expr_exp(symtable *scope, int static_ctx)
{
	expr *e;

	e = parse_expr_assignment(scope, static_ctx);

	if(accept(token_comma)){
		expr *ret = expr_new_comma();
		ret->lhs = e;
		ret->rhs = parse_expr_exp(scope, static_ctx);
		return ret;
	}
	return e;
}

expr **parse_funcargs(symtable *scope, int static_ctx)
{
	expr **args = NULL;

	if(curtok == token_close_paren)
		return NULL;

	do{
		expr *arg = PARSE_EXPR_NO_COMMA(scope, static_ctx);
		UCC_ASSERT(arg, "no arg?");
		dynarray_add(&args, arg);
	}while(accept(token_comma));

	return args;
}
