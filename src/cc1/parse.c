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
#include "data_store.h"

#define STAT_NEW(type)      stmt_new_wrapper(type, current_scope)
#define STAT_NEW_NEST(type) stmt_new_wrapper(type, symtab_new(current_scope))

stmt *parse_code_block(void);
stmt *parse_code(void);
expr **parse_funcargs(void);

expr *parse_expr_unary(void);
#define parse_expr_cast() parse_expr_unary()

/* parse_type uses this for structs, tdefs and enums */
symtable *current_scope;
static_assert **static_asserts;


/* sometimes we can carry on after an error, but we don't want to go through to compilation etc */
int parse_had_error = 0;


/* switch, for, do and while linkage */
static stmt *current_continue_target,
						*current_break_target,
						*current_switch;


expr *parse_expr_sizeof_typeof()
{
	expr *e;

	if(accept(token_open_paren)){
		decl *d = parse_decl_single(DECL_SPEL_NO);

		if(d){
			e = expr_new_sizeof_decl(d);
		}else{
			/* parse a full one, since we're in brackets */
			e = expr_new_sizeof_expr(parse_expr_exp());
		}

		EAT(token_close_paren);
	}else{
		e = expr_new_sizeof_expr(parse_expr_unary());
		/* don't go any higher, sizeof a - 1, means sizeof(a) - 1 */
	}

	return e;
}

expr *parse_expr__Generic()
{
	struct generic_lbl **lbls;
	expr *test;

	EAT(token__Generic);
	EAT(token_open_paren);

	test = parse_expr_no_comma();
	lbls = NULL;

	for(;;){
		decl *d;
		expr *e;
		struct generic_lbl *lbl;

		EAT(token_comma);

		if(accept(token_default)){
			d = NULL;
		}else{
			d = parse_decl_single(DECL_SPEL_NO);
			if(!d)
				die_at(NULL, "type expected");
		}
		EAT(token_colon);
		e = parse_expr_no_comma();

		lbl = umalloc(sizeof *lbl);
		lbl->e = e;
		lbl->d = d;
		dynarray_add((void ***)&lbls, lbl);

		if(accept(token_close_paren))
			break;
	}

	return expr_new__Generic(test, lbls);
}

expr *parse_expr_identifier()
{
	expr *e;

	if(curtok != token_identifier)
		die_at(NULL, "identifier expected, got %s (%s:%d)",
				token_to_str(curtok), __FILE__, __LINE__);

	e = expr_new_identifier(token_current_spel());
	EAT(token_identifier);
	return e;
}

expr *parse_expr_primary()
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
			int l;

			token_get_current_str(&s, &l);
			EAT(token_string);

			return expr_new_addr_data(data_store_new_str(s, l));
		}

		case token__Generic:
			return parse_expr__Generic();

		default:
			if(accept(token_open_paren)){
				decl *d;
				expr *e;

				if((d = parse_decl_single(DECL_SPEL_NO))){
					if(d->type->store)
						die_at(&d->where, "invalid cast involving %s",
								type_store_to_str(d->type->store));

					e = expr_new_cast(d);
					EAT(token_close_paren);
					e->expr = parse_expr_cast(); /* another cast */
					return e;

				}else if(curtok == token_open_block){
					/* ({ ... }) */
					e = expr_new_stmt(parse_code_block());
				}else{
					/* mark as being inside parens, for if((x = 5)) checking */
					e = parse_expr_exp();
					e->in_parens = 1;
				}

				EAT(token_close_paren);
				return e;
			}else{
				if(curtok != token_identifier){
					die_at(NULL, "expression expected, got %s (%s:%d)",
							token_to_str(curtok), __FILE__, __LINE__);
				}

				return parse_expr_identifier();
			}
			break;
	}
}

expr *parse_expr_postfix()
{
	expr *e;
	int flag;

	e = parse_expr_primary();

	for(;;){
		if(accept(token_open_square)){
			expr *sum, *deref;

			sum = expr_new_op(op_plus);

			sum->lhs  = e;
			sum->rhs  = parse_expr_exp();

			EAT(token_close_square);

			deref = expr_new_op(op_deref);
			deref->lhs  = sum;

			e = deref;

		}else if(accept(token_open_paren)){
			expr *fcall = expr_new_funcall();

			fcall->funcargs = parse_funcargs();
			fcall->expr = e;
			EAT(token_close_paren);

			e = fcall;

		}else if((flag = accept(token_dot)) || accept(token_ptr)){
			expr *st_op = expr_new_op(flag ? op_struct_dot : op_struct_ptr);

			st_op->lhs = e;
			st_op->rhs = parse_expr_identifier();

			e = st_op;

		}else if((flag = accept(token_increment)) || accept(token_decrement)){
			expr *op = expr_new_op(flag ? op_plus : op_minus);
			expr *inc = expr_new_assign(e, op);

			inc->assign_is_post = 1;

			op->lhs = e;
			op->rhs = expr_new_val(1);

			e = inc;
		}else{
			break;
		}
	}

	return e;
}

expr *parse_expr_unary()
{
	expr *e;
	int flag;

	if((flag = accept(token_increment)) || accept(token_decrement)){
		/* this is a normal increment, i.e. ++x, simply translate it to x = x + 1 */
		expr *op, *to;

		to = parse_expr_unary();
		op = expr_new_op(flag ? op_plus : op_minus);
		e = expr_new_assign(to, op);

		/* assign to... */
		op->lhs = e->lhs;
		op->rhs = expr_new_val(1);
		/*
		 * looks like this:
		 *
		 * e {
		 *   type = assign
		 *   lhs {
		 *     "varname"
		 *   }
		 *   rhs {
		 *     type = assign
		 *     op   = op_plus
		 *     lhs {
		 *       "varname"
		 *     }
		 *     rhs {
		 *       1
		 *     }
		 *   }
		 * }
		 */
	}else{
		switch(curtok){
			case token_and:
				e = expr_new_addr();
				goto do_parse;

			case token_multiply:
				e = expr_new_op(op_deref);
				goto do_parse;

			case token_plus:
			case token_minus:
			case token_bnot:
			case token_not:
				e = expr_new_op(curtok_to_op());
do_parse:
				EAT(curtok);
				e->lhs = parse_expr_cast();
				break;

			case token_sizeof:
				EAT(token_sizeof);
				e = parse_expr_sizeof_typeof();
				break;

			default:
				e = parse_expr_postfix();
		}
	}

	return e;
}

expr *parse_expr_generic(expr *(*above)(), enum token t, ...)
{
	expr *e = above();
	va_list l;

	va_start(l, t);
	while(curtok == t || curtok_in_list(l)){
		expr *join = expr_new_op(curtok_to_op());
		EAT(curtok);
		join->lhs = e;
		join->rhs = above();
		e = join;
	}

	va_end(l);
	return e;
}

#define PARSE_DEFINE(this, above, ...) \
expr *parse_expr_ ## this()            \
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

expr *parse_expr_conditional()
{
	expr *e = parse_expr_logical_or();

	if(accept(token_question)){
		expr *q = expr_new_if(e);

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

	e = parse_expr_conditional();

	if(accept(token_assign)){
		expr *from = parse_expr_assignment();
		expr *ret  = expr_new_assign(e, from);

		return ret;

	}else if(curtok_is_augmented_assignment()){
		/* +=, ... */
		expr *added;
		expr *ass;

		added = expr_new_op(curtok_to_augmented_op());
		ass = expr_new_assign(e, added);

		EAT(curtok);

		added->lhs = e;
		added->rhs = parse_expr_assignment();

		e = ass;
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

expr **parse_funcargs()
{
	expr **args = NULL;

	while(curtok != token_close_paren){
		expr *arg = parse_expr_no_comma();
		if(!arg)
			die_at(&arg->where, "expected: funcall arg");
		dynarray_add((void ***)&args, arg);

		if(curtok == token_close_paren)
			break;
		EAT(token_comma);
	}

	return args;
}

void parse_test_init_expr(stmt *t)
{
	decl **c99_ucc_inits;

	EAT(token_open_paren);

	c99_ucc_inits = parse_decls_one_type();
	if(c99_ucc_inits){
		t->flow = stmt_flow_new(symtab_new(t->symtab));

		current_scope = t->flow->for_init_symtab;

		t->flow->for_init_decls = c99_ucc_inits;
	}else{
		t->expr = parse_expr_exp();
	}

	EAT(token_close_paren);
}

stmt *parse_if()
{
	stmt *t = STAT_NEW(if);

	EAT(token_if);

	parse_test_init_expr(t);

	t->lhs = parse_code();

	if(accept(token_else))
		t->rhs = parse_code();

	if(t->flow)
		current_scope = current_scope->parent;

	return t;
}

stmt *expr_to_stmt(expr *e)
{
	stmt *t = STAT_NEW(expr);
	t->expr = e;
	return t;
}

stmt *parse_switch()
{
	stmt *t = STAT_NEW(switch);
	stmt *old = current_break_target;
	stmt *old_sw = current_switch;

	current_switch = current_break_target = t;

	EAT(token_switch);

	parse_test_init_expr(t);

	t->lhs = parse_code();

	current_break_target = old;
	current_switch = old_sw;

	return t;
}

stmt *parse_do()
{
	stmt *t = STAT_NEW(do);

	current_continue_target = current_break_target = t;

	EAT(token_do);

	t->lhs = parse_code();

	EAT(token_while);
	EAT(token_open_paren);
	t->expr = parse_expr_exp();
	EAT(token_close_paren);
	EAT(token_semicolon);

	return t;
}

stmt *parse_while()
{
	stmt *t = STAT_NEW(while);

	current_continue_target = current_break_target = t;

	EAT(token_while);

	parse_test_init_expr(t);

	t->lhs = parse_code();

	return t;
}

stmt *parse_for()
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
			if(c99inits)
				sf->for_init_decls = c99inits;
			else
				sf->for_init = parse_expr_exp()
	);

	SEMI_WRAP(sf->for_while = parse_expr_exp());

#undef SEMI_WRAP

	if(!accept(token_close_paren)){
		sf->for_inc   = parse_expr_exp();
		EAT(token_close_paren);
	}

	s->lhs = parse_code();

	current_scope = current_scope->parent;

	return s;
}

void parse_static_assert(void)
{
	while(accept(token__Static_assert)){
		static_assert *sa = umalloc(sizeof *sa);
		int l;

		sa->scope = current_scope;

		EAT(token_open_paren);
		sa->e = parse_expr_no_comma();
		EAT(token_comma);

		token_get_current_str(&sa->s, &l);

		EAT(token_string);
		EAT(token_close_paren);
		EAT(token_semicolon);

		dynarray_add((void ***)&static_asserts, sa);
	}
}


stmt *parse_code_block()
{
	stmt *t = STAT_NEW_NEST(code);
	decl **diter;

	current_scope = t->symtab;

	EAT(token_open_block);

	if(accept(token_close_block))
		goto ret;

	t->decls = parse_decls_multi_type(DECL_MULTI_ACCEPT_FUNC_DECL);

	for(diter = t->decls; diter && *diter; diter++){
		/* only extract the init if it's not static */
		decl *d = *diter;
		if(d->init){
			if(decl_is_array(d)){
#ifndef FANCY_STACK_INIT
				ICE("TODO");
				/* assignment expr for each init */
				array_decl *dinit = d->init->array_store;
				int i;
				expr *comma_init = NULL;

				for(i = 0; i < dinit->len; i++){
					int init_val;
					expr *init;

					if(dinit->type == array_exprs)
						init_val = dinit->data.exprs[i]->val.iv.val;
					else
						init_val = dinit->data.str[i];

					init = expr_new_array_decl_init(d, init_val, i);

					if(comma_init){
						expr *new = expr_new_comma();
						new->lhs = comma_init;
						new->rhs = init;
						comma_init = new;
					}else{
						comma_init = init;
					}
				}

				dynarray_add((void ***)&t->codes, expr_to_stmt(comma_init));
#endif
			}else if(d->type->store != store_static && d->init->type == decl_init_scalar){
				dynarray_add((void ***)&t->codes, expr_to_stmt(d->init->bits.expr));
			}else{
				ICW("TODO: init for %s somewhere else", d->spel);
			}
		}
		/* don't change init - used for checks for assign-to-const */
	}

	if(curtok != token_close_block){
		/* main read loop */
		do{
			stmt *sub;

			parse_static_assert();

			sub = parse_code();

			if(sub)
				dynarray_add((void ***)&t->codes, sub);
			else
				break;
		}while(curtok != token_close_block);
	}
	/*
	 * else:
	 * { int i; }
	 */

	EAT(token_close_block);
ret:
	current_scope = t->symtab->parent;
	return t;
}

stmt *parse_label_next(stmt *lbl)
{
	lbl->lhs = parse_code();
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

stmt *parse_code()
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
				EAT(token_goto);

				t = STAT_NEW(goto);
				t->expr = parse_expr_identifier();
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

		case token_open_block: return parse_code_block();

		case token_switch:
			return parse_switch();

		case token_default:
			EAT(token_default);
			EAT(token_colon);
			t = STAT_NEW(default);
			t->parent = current_switch;
			return parse_label_next(t);
		case token_case:
		{
			expr *a;
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
			return parse_label_next(t);
		}

		default:
			t = expr_to_stmt(parse_expr_exp());

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

symtable *parse()
{
	symtable *globals;
	decl **decls = NULL;
	int i;

	current_scope = globals = symtab_new(NULL);

	decls = parse_decls_multi_type(DECL_MULTI_CAN_DEFAULT | DECL_MULTI_ACCEPT_FUNC_CODE);
	EAT(token_eof);

	if(parse_had_error)
		exit(1);

	if(decls)
		for(i = 0; decls[i]; i++)
			symtab_add(globals, decls[i], sym_global, SYMTAB_NO_SYM, SYMTAB_APPEND);

	globals->static_asserts = static_asserts;

	return globals;
}
