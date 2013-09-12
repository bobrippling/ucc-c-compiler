#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "../util/util.h"
#include "data_structs.h"
#include "tokenise.h"
#include "parse.h"
#include "../util/alloc.h"
#include "tokconv.h"
#include "../util/util.h"
#include "sym.h"
#include "cc1.h"
#include "../util/dynarray.h"
#include "sue.h"
#include "parse_type.h"
#include "const.h"
#include "ops/__builtin.h"
#include "funcargs.h"

#define STAT_NEW(type)      stmt_new_wrapper(type, current_scope)
#define STAT_NEW_NEST(type) stmt_new_wrapper(type, symtab_new(current_scope))

#define STMT_SCOPE_RECOVER(t)              \
	if(t->flow)                              \
		current_scope = current_scope->parent; \
	if(cc1_std == STD_C99)                   \
		current_scope = current_scope->parent
/* ^ recover C99's if/switch/while scoping */

stmt *parse_stmt_block(void);
stmt *parse_stmt(void);
expr **parse_funcargs(void);

expr *parse_expr_unary(void);
#define parse_expr_cast() parse_expr_unary()

/* parse_type uses this for structs, tdefs and enums */
symtable *current_scope;


/* sometimes we can carry on after an error, but we don't want to go through to compilation etc */
int parse_had_error = 0;


/* switch, for, do and while linkage */
static stmt *current_continue_target,
						*current_break_target,
						*current_switch;


expr *parse_expr_sizeof_typeof_alignof(enum what_of what_of)
{
	expr *e;
	where w;

	where_cc1_current(&w);
	w.chr -= what_of == what_alignof ? 7 : 6; /* go back over the *of */

	if(accept(token_open_paren)){
		type_ref *r = parse_type();

		if(r){
			EAT(token_close_paren);

			/* check for sizeof(int){...} */
			if(curtok == token_open_block)
				e = expr_new_sizeof_expr(
							expr_new_compound_lit(r,
								parse_initialisation()),
							what_of);
			else
				e = expr_new_sizeof_type(r, what_of);

		}else{
			/* parse a full one, since we're in brackets */
			e = expr_new_sizeof_expr(parse_expr_exp(), what_of);
			EAT(token_close_paren);
		}

	}else{
		if(what_of == what_typeof)
			/* TODO? cc1_error = 1, return expr_new_val(0) */
			die_at(NULL, "open paren expected after typeof");

		e = expr_new_sizeof_expr(parse_expr_unary(), what_of);
		/* don't go any higher, sizeof a - 1, means sizeof(a) - 1 */
	}

	return expr_set_where_len(e, &w);
}

static expr *parse_expr__Generic()
{
	struct generic_lbl **lbls;
	expr *test;
	where w;

	where_cc1_current(&w);

	EAT(token__Generic);
	EAT(token_open_paren);

	test = parse_expr_no_comma();
	lbls = NULL;

	for(;;){
		type_ref *r;
		expr *e;
		struct generic_lbl *lbl;

		EAT(token_comma);

		if(accept(token_default)){
			r = NULL;
		}else{
			r = parse_type();
			if(!r)
				die_at(NULL, "type expected");
		}
		EAT(token_colon);
		e = parse_expr_no_comma();

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

static expr *parse_expr_identifier()
{
	expr *e;

	if(curtok != token_identifier)
		die_at(NULL, "identifier expected, got %s (%s:%d)",
				token_to_str(curtok), __FILE__, __LINE__);

	e = expr_new_identifier(token_current_spel());
	where_cc1_adj_identifier(&e->where, e->bits.ident.spel);
	EAT(token_identifier);
	return e;
}

static expr *parse_block()
{
	funcargs *args;
	type_ref *rt;

	EAT(token_xor);

	rt = parse_type();

	if(rt){
		if(type_ref_is(rt, type_ref_func)){
			/* got ^int (args...) */
			rt = type_ref_func_call(rt, &args);
		}else{
			/* ^int {...} */
			goto def_args;
		}

	}else if(accept(token_open_paren)){
		/* ^(args...) */
		args = parse_func_arglist();
		EAT(token_close_paren);
	}else{
		/* ^{...} */
def_args:
		args = funcargs_new();
		args->args_void = 1;
	}

	return expr_new_block(rt, args, parse_stmt_block());
}

static expr *parse_expr_primary()
{
	switch(curtok){
		case token_integer:
		case token_character:
		{
			expr *e = expr_new_intval(&currentval);
			EAT(curtok);
			return e;
		}

		case token_string:
		/*case token_open_block: - not allowed here */
		{
			char *s;
			int l, wide;

			token_get_current_str(&s, &l, &wide);
			EAT(token_string);

			return expr_new_str(s, l, wide);
		}

		case token__Generic:
			return parse_expr__Generic();

		case token_xor:
			return parse_block();

		default:
		{
			where loc_start;

			if(accept_where(token_open_paren, &loc_start)){
				type_ref *r;
				expr *e;

				if((r = parse_type())){
					EAT(token_close_paren);

					if(curtok == token_open_block){
						/* C99 compound lit. */
						e = expr_new_compound_lit(
								r, parse_initialisation());

					}else{
						/* another cast */
						e = expr_new_cast(
								parse_expr_cast(), r, 0);
					}

					return expr_set_where_len(e, &loc_start);

				}else if(curtok == token_open_block){
					/* ({ ... }) */
					e = expr_new_stmt(parse_stmt_block());
				}else{
					/* mark as being inside parens, for if((x = 5)) checking */
					e = parse_expr_exp();
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
					die_at(NULL, "expression expected, got %s (%s:%d)",
							token_to_str(curtok), __FILE__, __LINE__);
				}

				return parse_expr_identifier();
			}
			/* unreachable */
		}
	}
}

static expr *parse_expr_postfix()
{
	expr *e;
	int flag;

	e = parse_expr_primary();

	for(;;){
		where w;

		if(accept(token_open_square)){
			expr *sum = expr_new_op(op_plus);

			sum->lhs  = e;
			sum->rhs  = parse_expr_exp();

			EAT(token_close_square);

			e = expr_new_deref(sum);

		}else if(accept(token_open_paren)){
			expr *fcall = NULL;

			/* check for specialised builtin parsing */
			if(expr_kind(e, identifier))
				fcall = builtin_parse(e->bits.ident.spel);

			if(!fcall){
				fcall = expr_new_funcall();
				fcall->funcargs = parse_funcargs();
			}

			fcall->expr = e;
			EAT(token_close_paren);

			e = fcall;

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

expr *parse_expr_unary()
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
					parse_expr_unary(), /* lval */
					expr_new_val(1),
					flag ? op_plus : op_minus);

	}else{
		switch(curtok){
			case token_andsc:
				/* GNU &&label */
				EAT(curtok);
				e = expr_new_addr_lbl(token_current_spel());
				EAT(token_identifier);
				break;

			case token_and:
				EAT(token_and);
				e = expr_new_addr(parse_expr_cast());
				break;

			case token_multiply:
				EAT(curtok);
				e = expr_new_deref(parse_expr_cast());
				break;

			case token_plus:
			case token_minus:
			case token_bnot:
			case token_not:
				e = expr_new_op(curtok_to_op());
				EAT(curtok);
				e->lhs = parse_expr_cast();
				break;

			case token_sizeof:
				set_w = 0; /* no need since there's no sub-parsing here */
				EAT(token_sizeof);
				e = parse_expr_sizeof_typeof_alignof(what_sizeof);
				break;

			case token__Alignof:
				set_w = 0;
				EAT(token__Alignof);
				e = parse_expr_sizeof_typeof_alignof(what_alignof);
				break;

			default:
				set_w = 0;
				e = parse_expr_postfix();
		}
	}

	if(set_w)
		expr_set_where(e, &w);

	return e;
}

static expr *parse_expr_generic(expr *(*above)(), enum token t, ...)
{
	expr *e = above();

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
		join->rhs = above();

		e = join;
	}

	return e;
}

#define PARSE_DEFINE(this, above, ...) \
static expr *parse_expr_ ## this()     \
{                                      \
	return parse_expr_generic(           \
			parse_expr_ ## above,            \
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

static expr *parse_expr_conditional()
{
	expr *e = parse_expr_logical_or();
	where w;

	if(accept_where(token_question, &w)){
		expr *q = expr_set_where(expr_new_if(e), &w);

		if(accept(token_colon)){
			q->lhs = NULL; /* sentinel */
		}else{
			q->lhs = parse_expr_exp();
			EAT(token_colon);
		}
		q->rhs = parse_expr_conditional();

		return q;
	}
	return e;
}

expr *parse_expr_assignment()
{
	expr *e;
	where w;

	e = parse_expr_conditional();

	if(accept_where(token_assign, &w)){
		return expr_set_where(
				expr_new_assign(e, parse_expr_assignment()),
				&w);

	}else if(curtok_is_compound_assignment()){
		/* +=, ... - only evaluate the lhs once*/
		enum op_type op = curtok_to_compound_op();

		where_cc1_current(&w);
		EAT(curtok);

		return expr_set_where(
				expr_new_assign_compound(e,
					parse_expr_assignment(),
					op),
				&w);
	}

	return e;
}

expr *parse_expr_exp()
{
	expr *e;

	e = parse_expr_assignment();

	if(accept(token_comma)){
		expr *ret = expr_new_comma();
		ret->lhs = e;
		ret->rhs = parse_expr_exp();
		return ret;
	}
	return e;
}

type_ref **parse_type_list()
{
	type_ref **types = NULL;

	if(curtok == token_close_paren)
		return types;

	do{
		type_ref *r = parse_type();

		if(!r)
			die_at(NULL, "type expected");

		dynarray_add(&types, r);
	}while(accept(token_comma));

	return types;
}

expr **parse_funcargs()
{
	expr **args = NULL;

	while(curtok != token_close_paren){
		expr *arg = parse_expr_no_comma();
		UCC_ASSERT(arg, "no arg?");
		dynarray_add(&args, arg);

		if(curtok == token_close_paren)
			break;
		EAT(token_comma);
	}

	return args;
}

static void parse_test_init_expr(stmt *t)
{
	decl *d;

	EAT(token_open_paren);

	/* if C99, we create a new scope here, for e.g.
	 * if(5 > (enum { a, b })a){ return a; } return b;
	 * "return b" can't see 'b' since its scope is only the if
	 *
	 * C90 drags the scope of the enum up to the enclosing block
	 */
	if(cc1_std == STD_C99)
		t->symtab = current_scope = symtab_new(current_scope);

	d = parse_decl_single(DECL_SPEL_NEED);
	if(d){
		t->flow = stmt_flow_new(symtab_new(current_scope));

		current_scope = t->flow->for_init_symtab;

		dynarray_add(&current_scope->decls, d);

		if(accept(token_comma)){
			/* if(int i = 5, i > f()){ ... } */
			t->expr = parse_expr_exp();
		}else{
			/* if(int i = 5) -> if(i) */
			t->expr = expr_new_identifier(d->spel);
		}
	}else{
		t->expr = parse_expr_exp();
	}

	EAT(token_close_paren);
}

static stmt *parse_if()
{
	stmt *t = STAT_NEW(if);

	EAT(token_if);

	parse_test_init_expr(t);

	t->lhs = parse_stmt();

	if(accept(token_else))
		t->rhs = parse_stmt();

	STMT_SCOPE_RECOVER(t);

	return t;
}

static stmt *parse_switch()
{
	stmt *t = STAT_NEW(switch);
	stmt *old = current_break_target;
	stmt *old_sw = current_switch;

	current_switch = current_break_target = t;

	EAT(token_switch);

	parse_test_init_expr(t);

	t->lhs = parse_stmt();

	current_break_target = old;
	current_switch = old_sw;

	STMT_SCOPE_RECOVER(t);

	return t;
}

static stmt *parse_do()
{
	stmt *t = STAT_NEW(do);

	current_continue_target = current_break_target = t;

	EAT(token_do);

	t->lhs = parse_stmt();

	EAT(token_while);
	EAT(token_open_paren);
	t->expr = parse_expr_exp();
	EAT(token_close_paren);
	EAT(token_semicolon);

	return t;
}

static stmt *parse_while()
{
	stmt *t = STAT_NEW(while);

	current_continue_target = current_break_target = t;

	EAT(token_while);

	parse_test_init_expr(t);

	t->lhs = parse_stmt();

	STMT_SCOPE_RECOVER(t);

	return t;
}

static stmt *parse_for()
{
	stmt *s = STAT_NEW(for);
	stmt_flow *sf;

	current_continue_target = current_break_target = s;

	EAT(token_for);
	EAT(token_open_paren);

	sf = s->flow = stmt_flow_new(symtab_new(s->symtab));

	current_scope = sf->for_init_symtab;

#define SEMI_WRAP(code)         \
	if(!accept(token_semicolon)){ \
		code;                       \
		EAT(token_semicolon);       \
	}

	SEMI_WRAP(
			decl **c99inits = parse_decls_one_type();
			if(c99inits){
				dynarray_add_array(&current_scope->decls, c99inits);

				if(cc1_std < STD_C99)
					warn_at(NULL, "use of C99 for-init");
			}else{
				sf->for_init = parse_expr_exp();
			}
	);

	SEMI_WRAP(sf->for_while = parse_expr_exp());

#undef SEMI_WRAP

	if(!accept(token_close_paren)){
		sf->for_inc   = parse_expr_exp();
		EAT(token_close_paren);
	}

	s->lhs = parse_stmt();

	current_scope = current_scope->parent;

	return s;
}

void parse_static_assert(void)
{
	while(accept(token__Static_assert)){
		static_assert *sa = umalloc(sizeof *sa);

		sa->scope = current_scope;

		EAT(token_open_paren);
		sa->e = parse_expr_no_comma();
		EAT(token_comma);

		token_get_current_str(&sa->s, NULL, NULL);

		EAT(token_string);
		EAT(token_close_paren);
		EAT(token_semicolon);

		dynarray_add(&current_scope->static_asserts, sa);
	}
}

static stmt *parse_stmt_and_decls(void)
{
	stmt *code_stmt = STAT_NEW_NEST(code);

	current_scope = code_stmt->symtab;

	parse_static_assert();

	parse_decls_multi_type(
			DECL_MULTI_ACCEPT_FUNC_DECL
			| DECL_MULTI_ALLOW_STORE
			| DECL_MULTI_ALLOW_ALIGNAS,
			current_scope,
			NULL);

	if(curtok != token_close_block){
		/* fine with a normal statement */
		int at_decl = 0;

		for(;;){
			parse_static_assert();
			if(curtok == token_close_block || (at_decl = parse_at_decl()))
				break;
			dynarray_add(&code_stmt->codes, parse_stmt());
		}

		if(at_decl){
			if(code_stmt->codes){
				stmt *nest = parse_stmt_and_decls();

				if(cc1_std < STD_C99){
					static int warned = 0;
					if(!warned){
						warned = 1;
						cc1_warn_at(&nest->where, 0, WARN_MIXED_CODE_DECLS,
								"mixed code and declarations");
					}
				}

				/* mark as internal - for duplicate checks */
				nest->symtab->internal_nest = 1;

				dynarray_add(&code_stmt->codes, nest);
			}else{
				ICE("got another decl - should've been handled already");
			}
		}
	}

	/*current_scope = code_stmt->symtab->parent;
	 * don't - we use ->parent for scope leak checks
	 */
	current_scope = current_scope->parent;

	where_cc1_current(&code_stmt->where_cbrace);

	return code_stmt;
}

#ifdef SYMTAB_DEBUG
static int print_indent = 0;

#define INDENT(...) indent(), fprintf(stderr, __VA_ARGS__)

void indent()
{
	for(int c = print_indent; c; c--)
		fputc('\t', stderr);
}

void print_stmt_and_decls(stmt *t)
{
	INDENT("decls: (symtab %p, parent %p)\n", t->symtab, t->symtab->parent);

	print_indent++;
	for(decl **i = t->decls; i && *i; i++)
		INDENT("%s\n", (*i)->spel);
	print_indent--;

	if(!t->decls)
		print_indent++, INDENT("NONE\n"), print_indent--;

	INDENT("codes:\n");
	for(stmt **i = t->codes; i && *i; i++){
		stmt *s = *i;
		if(stmt_kind(s, code)){
			print_indent++;
			INDENT("more-codes:\n");
			print_indent++;
			print_stmt_and_decls(s);
			print_indent -= 2;
		}else{
			print_indent++;
			INDENT("%s%s%s (symtab %p)\n",
					s->f_str(),
					stmt_kind(s, expr) ? ": " : "",
					stmt_kind(s, expr) ? s->expr->f_str() : "",
					s->symtab);
			print_indent--;
		}
	}
	if(!t->codes)
		print_indent++, INDENT("NONE\n"), print_indent--;
}
#endif

stmt *parse_stmt_block()
{
	stmt *t;

	EAT(token_open_block);

	t = parse_stmt_and_decls();

	EAT(token_close_block);

#ifdef SYMTAB_DEBUG
	fprintf(stderr, "Parsed statement block:\n");
	print_stmt_and_decls(t);
#endif

	return t;
}

static stmt *parse_label_next(stmt *lbl)
{
	lbl->lhs = parse_stmt();
	/*
	 * a label must have a block of code after it:
	 *
	 * if(x)
	 *   lbl:
	 *   printf("yo\n");
	 *
	 * both the label and the printf statements are in the if
	 * as a compound statement
	 */
	return lbl;
}

stmt *parse_stmt()
{
	stmt *t;

	switch(curtok){
		case token_semicolon:
			t = STAT_NEW(noop);
			EAT(token_semicolon);
			return t;

		case token_break:
		case token_continue:
		case token_goto:
		case token_return:
		{
			if(accept(token_break)){
				t = STAT_NEW(break);
				t->parent = current_break_target;

			}else if(accept(token_continue)){
				t = STAT_NEW(continue);
				t->parent = current_continue_target;

			}else if(accept(token_return)){
				t = STAT_NEW(return);

				if(curtok != token_semicolon)
					t->expr = parse_expr_exp();
			}else{
				t = STAT_NEW(goto);

				EAT(token_goto);

				if(accept(token_multiply)){
					/* computed goto */
					t->expr = parse_expr_exp();
					t->expr->expr_computed_goto = 1;
				}else{
					t->expr = parse_expr_identifier();
				}
			}
			EAT(token_semicolon);
			return t;
		}

		case token_if:
			return parse_if();

		{
			stmt *(*parse_f)();
			stmt *ret, *old[2];

		case token_while:
			parse_f = parse_while;
			goto flow;
		case token_do:
			parse_f = parse_do;
			goto flow;
		case token_for:
			parse_f = parse_for;
			goto flow;

flow:
			old[0] = current_continue_target;
			old[1] = current_break_target;

			ret = parse_f();

			current_continue_target = old[0];
			current_break_target    = old[1];

			return ret;
		}

		case token_open_block: return parse_stmt_block();

		case token_switch:
			return parse_switch();

		case token_default:
			t = STAT_NEW(default);
			EAT(token_default);
			EAT(token_colon);
			t->parent = current_switch;
			return parse_label_next(t);
		case token_case:
		{
			expr *a;
			where cse_loc;
			where_cc1_current(&cse_loc);

			EAT(token_case);
			a = parse_expr_exp();
			if(accept(token_elipsis)){
				t = STAT_NEW(case_range);
				t->parent = current_switch;
				t->expr  = a;
				t->expr2 = parse_expr_exp();
			}else{
				t = STAT_NEW(case);
				t->expr = a;
				t->parent = current_switch;
			}

			EAT(token_colon);
			return stmt_set_where(parse_label_next(t), &cse_loc);
		}

		default:
			t = expr_to_stmt(parse_expr_exp(), current_scope);

			if(expr_kind(t->expr, identifier) && accept(token_colon)){
				stmt_mutate_wrapper(t, label);
				return parse_label_next(t);
			}else{
				EAT(token_semicolon);
				return t;
			}
	}

	/* unreachable */
}

symtable_gasm *parse_gasm(void)
{
	symtable_gasm *g = umalloc(sizeof *g);

	EAT(token_open_paren);
	token_get_current_str(&g->asm_str, NULL, NULL);
	EAT(token_string);
	EAT(token_close_paren);
	EAT(token_semicolon);

	return g;
}
