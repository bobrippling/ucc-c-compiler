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
#include "struct.h"
#include "parse_type.h"

#define STAT_NEW(type)      stmt_new_wrapper(type, current_scope)
#define STAT_NEW_NEST(type) stmt_new_wrapper(type, symtab_new(current_scope))

stmt *parse_code_block(void);
expr *parse_expr_funcall(void);
expr *parse_expr_inc_dec(void);

/*
 * order goes:
 *
 * parse_expr_unary_op   [-+!~]`parse_expr_single`
 * parse_expr_binary_op  above [/%*]    above
 * parse_expr_sum        above [+-]     above
 * parse_expr_shift      above [<<|>>]  above
 * parse_expr_bit_op     above [&|^]    above
 * parse_expr_cmp_op     above [><==!=] above
 * parse_expr_logical_op above [&&||]   above
 *
 * yeah yeah this has changed since i started
 * grep '^expr \+\*' $f
 * will do the trick
 */

/* parse_type uses this for structs, tdefs and enums */
symtable *current_scope;


expr *parse_lone_identifier()
{
	expr *e;

	if(curtok != token_identifier)
		EAT(token_identifier); /* raise error */

	e = expr_new_identifier(token_current_spel());
	EAT(token_identifier);

	return e;
}

expr *parse_expr_unary_op()
{
	expr *e;

	switch(curtok){
		case token_sizeof:
			EAT(token_sizeof);
			e = expr_new_sizeof();

			if(accept(token_open_paren)){
				decl *d = parse_decl_single(DECL_SPEL_NO);

				if(d){
					e->tree_type = d;
				}else{
					/* parse a full one, since we're in brackets */
					e->expr = parse_expr();
				}

				e->expr->expr_is_sizeof = !!d;

				EAT(token_close_paren);
			}else{
				e->expr = parse_expr_deref();
				/* don't go any higher, sizeof a - 1, means sizeof(a) - 1 */
			}
			return e;

		case token_integer:
		case token_character:
			e = expr_new_intval(&currentval);
			EAT(curtok);
			return e;

		case token_and:
			EAT(token_and);
			e = expr_new_addr();
			e->expr = parse_expr_array();
			return e;

		case token_string:
		case token_open_block: /* array */
			e = expr_new_addr();

			e->array_store = array_decl_new();

			if(curtok == token_string){
				char *s;
				int l;

				token_get_current_str(&s, &l);
				EAT(token_string);

				e->array_store->data.str = s;
				e->array_store->len      = l;

				e->array_store->type = array_str;
			}else{
				EAT(token_open_block);
				for(;;){
					dynarray_add((void ***)&e->array_store->data.exprs, parse_expr_funcallarg());
					if(accept(token_comma)){
						if(accept(token_close_block)) /* { 1, } */
							break;
						continue;
					}else{
						EAT(token_close_block);
						break;
					}
				}

				e->array_store->len = dynarray_count((void *)e->array_store->data.exprs);

				e->array_store->type = array_exprs;
			}
			return e;

		case token_open_paren:
		{
			decl *d;

			EAT(token_open_paren);

			if((d = parse_decl_single(DECL_SPEL_NO))){
				e = expr_new_cast(d);
				EAT(token_close_paren);
				e->expr = parse_expr_funcall(); /* grab only the closest */
				return e;
			}

			if(curtok == token_open_block){
				/* ({ ... }) */
				e = expr_new_stmt(parse_code_block());
			}else{
				e = parse_expr();
				e->in_parens = 1;
				/* mark as being inside parens, for if((x = 5)) checking */
			}
			EAT(token_close_paren);
			return e;
		}

		case token_plus:
			EAT(token_plus);
			return parse_expr();

		case token_minus:
			e = expr_new_op(curtok_to_op());
			EAT(token_minus);
			e->lhs = parse_expr_binary_op();
			return e;

		case token_not:
		case token_bnot:
			e = expr_new_op(curtok_to_op());

			EAT(curtok);
			e->lhs = parse_expr_binary_op();
			return e;

		case token_increment:
		case token_decrement:
		{
			const int inc = curtok == token_increment;
			/* this is a normal increment, i.e. ++x, simply translate it to x = x + 1 */
			e = expr_new_assign();
			EAT(curtok);

			/* assign to... */
			e->lhs = parse_expr_array();
			e->rhs = expr_new_op(inc ? op_plus : op_minus);
			e->rhs->lhs = e->lhs;
			e->rhs->rhs = expr_new_val(1);
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
			return e;
		}

		case token_identifier:
			return parse_lone_identifier();

		default:
			die_at(NULL, "expected: unary expression, got %s", token_to_str(curtok));
	}
	return NULL;
}

/* generalised recursive descent */
expr *parse_expr_join(expr *(*above)(), enum token accept, ...)
{
	va_list l;
	expr *e = above();

	for(;;){
		va_start(l, accept);
		if(curtok == accept || curtok_in_list(l)){
			expr *join;

			va_end(l);

			join       = expr_new_op(curtok_to_op());
			join->lhs  = e;

			EAT(curtok);
			join->rhs = above();

			e = join;
		}else{
			va_end(l);
			break;
		}
	}

	return e;
}

expr *parse_str_expruct()
{
	return parse_expr_join(parse_expr_unary_op,
				token_ptr, token_dot,
				token_unknown);
}

expr *parse_expr_array()
{
	expr *base = parse_str_expruct();

	while(accept(token_open_square)){
		expr *sum, *deref;

		sum = expr_new_op(op_plus);

		sum->lhs  = base;
		sum->rhs  = parse_expr();

		EAT(token_close_square);

		deref = expr_new_op(op_deref);
		deref->lhs  = sum;


		base = deref;
	}

	return base;
}

expr *parse_expr_inc_dec()
{
	expr *e = parse_expr_array();
	int flag = 0;

	if((flag = accept(token_increment)) || accept(token_decrement)){
		expr *inc = expr_new_assign();
		inc->assign_is_post = 1;

		inc->lhs = e;
		inc->rhs = expr_new_op(flag ? op_plus : op_minus);
		inc->rhs->lhs = e;
		inc->rhs->rhs = expr_new_val(1);
		e = inc;
	}

	return e;
}

expr *parse_expr_funcall()
{
	expr *e = parse_expr_inc_dec();

	while(accept(token_open_paren)){
		expr *sub = e;
		e = expr_new_funcall();
		e->funcargs = parse_funcargs();
		e->expr = sub;
		EAT(token_close_paren);
	}

	return e;
}

expr *parse_expr_deref()
{
	if(accept(token_multiply)){
		expr *e = expr_new_op(op_deref);
		e->lhs  = parse_expr_deref();
		return e;
	}

	return parse_expr_funcall();
}

expr *parse_expr_assign()
{
#define above parse_expr
	expr *e;

	e = parse_expr_deref();

	if(accept(token_assign)){
		e = expr_assignment(e, above());
	}else if(curtok_is_augmented_assignment()){
		/* +=, ... */
		expr *ass = expr_new_assign();

		ass->lhs = e;
		ass->rhs = expr_new_op(curtok_to_augmented_op());
		EAT(curtok);

		ass->rhs->lhs = e;
		ass->rhs->rhs = above();

		e = ass;
	}

	return e;
#undef above
}

expr *parse_expr_binary_op()
{
	/* above [/%*] above */
	return parse_expr_join(parse_expr_assign,
				token_multiply, token_divide, token_modulus,
				token_unknown);
}

expr *parse_expr_sum()
{
	/* above [+-] above */
	return parse_expr_join(parse_expr_binary_op,
			token_plus, token_minus, token_unknown);
}

expr *parse_expr_shift()
{
	/* above *shift* above */
	return parse_expr_join(parse_expr_sum,
				token_shiftl, token_shiftr, token_unknown);
}

expr *parse_expr_bit_op()
{
	/* above [&|^] above */
	return parse_expr_join(parse_expr_shift,
				token_and, token_or, token_xor, token_unknown);
}

expr *parse_expr_cmp_op()
{
	/* above [><==!=] above */
	return parse_expr_join(parse_expr_bit_op,
				token_eq, token_ne,
				token_le, token_lt,
				token_ge, token_gt,
				token_unknown);
}

expr *parse_expr_logical_op()
{
	/* above [&&||] above */
	return parse_expr_join(parse_expr_cmp_op,
			token_orsc, token_andsc, token_unknown);
}

expr *parse_expr_if()
{
	expr *e = parse_expr_logical_op();
	if(accept(token_question)){
		expr *q = expr_new_if(e);

		if(accept(token_colon)){
			q->lhs = NULL; /* sentinel */
		}else{
			q->lhs = parse_expr();
			EAT(token_colon);
		}
		q->rhs = parse_expr_funcallarg();

		return q;
	}else{
		return e;
	}
}

expr *parse_expr_comma()
{
	expr *e;

	e = parse_expr_funcallarg();

	if(accept(token_comma)){
		expr *ret = expr_new_comma();
		ret->lhs = e;
		ret->rhs = parse_expr_comma();
		return ret;
	}
	return e;
}

stmt *parse_if()
{
	stmt *t = STAT_NEW_NEST(if);
	EAT(token_if);
	EAT(token_open_paren);

	t->expr = parse_expr();

	EAT(token_close_paren);

	t->lhs = parse_code();

	if(accept(token_else))
		t->rhs = parse_code();

	return t;
}

expr **parse_funcargs()
{
	expr **args = NULL;

	while(curtok != token_close_paren){
		expr *arg = parse_expr_funcallarg();
		if(!arg)
			die_at(&arg->where, "expected: funcall arg");
		dynarray_add((void ***)&args, arg);

		if(curtok == token_close_paren)
			break;
		EAT(token_comma);
	}

	return args;
}


stmt *expr_to_stmt(expr *e)
{
	stmt *t = STAT_NEW(expr);
	t->expr = e;
	return t;
}

stmt *parse_switch()
{
	stmt *t = STAT_NEW_NEST(switch);

	EAT(token_switch);
	EAT(token_open_paren);

	t->expr = parse_expr();

	EAT(token_close_paren);

	t->lhs = parse_code();

	return t;
}

stmt *parse_do()
{
	stmt *t = STAT_NEW_NEST(do);

	EAT(token_do);

	t->lhs = parse_code();

	EAT(token_while);
	EAT(token_open_paren);
	t->expr = parse_expr();
	EAT(token_close_paren);
	EAT(token_semicolon);

	return t;
}

stmt *parse_while()
{
	stmt *t = STAT_NEW_NEST(while);

	EAT(token_while);
	EAT(token_open_paren);

	t->expr = parse_expr();
	EAT(token_close_paren);
	t->lhs = parse_code();

	return t;
}

stmt *parse_for()
{
	stmt *s = STAT_NEW_NEST(for);
	stmt_flow *sf;

	EAT(token_for);
	EAT(token_open_paren);

	sf = s->flow = stmt_flow_new();

#define SEMI_WRAP(code) \
	if(!accept(token_semicolon)){ \
		code; \
		EAT(token_semicolon); \
	}

	SEMI_WRAP(sf->for_init  = parse_expr());
	SEMI_WRAP(sf->for_while = parse_expr());

#undef SEMI_WRAP

	if(!accept(token_close_paren)){
		sf->for_inc   = parse_expr();
		EAT(token_close_paren);
	}

	s->lhs = parse_code();

	return s;
}

stmt *parse_code_block()
{
	stmt *t = STAT_NEW_NEST(code);
	decl **diter;

	current_scope = t->symtab;

	EAT(token_open_block);

	if(accept(token_close_block))
		goto ret;

	t->decls = PARSE_DECLS();

	for(diter = t->decls; diter && *diter; diter++)
		/* only extract the init if it's not static */
		if((*diter)->init && ((*diter)->type->spec & spec_static) == 0){
			expr *e;

			e = expr_new_identifier((*diter)->spel);

			dynarray_add((void ***)&t->codes, expr_to_stmt(expr_assignment(e, (*diter)->init)));

			/*
			 *(*diter)->init = NULL;
			 * leave it set, so we can check later in, say, fold.c for const init
			 */
		}

	if(curtok != token_close_block){
		/* main read loop */
		do{
			stmt *sub = parse_code();

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
	 * both the label and the printf stmtements are in the if
	 * as a compound stmtement
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
		case token_return:
		case token_goto:
			if(accept(token_break)){
				t = STAT_NEW(break);
			}else if(accept(token_return)){
				t = STAT_NEW(return);
				if(curtok != token_semicolon)
					t->expr = parse_expr();
			}else{
				EAT(token_goto);

				t = STAT_NEW(goto);
				t->expr = parse_lone_identifier();
			}
			EAT(token_semicolon);
			return t;

		case token_if:     return parse_if();
		case token_while:  return parse_while();
		case token_do:     return parse_do();
		case token_for:    return parse_for();

		case token_open_block: return parse_code_block();

		case token_switch:
			return parse_switch();
		case token_default:
			EAT(token_default);
			EAT(token_colon);
			t = STAT_NEW(default);
			return parse_label_next(t);
		case token_case:
		{
			expr *a;
			EAT(token_case);
			a = parse_expr();
			if(accept(token_elipsis)){
				t = STAT_NEW(case_range);
				t->expr  = a;
				t->expr2 = parse_expr();
			}else{
				t = expr_to_stmt(a);
				stmt_mutate_wrapper(t, case);
			}
			EAT(token_colon);
			return parse_label_next(t);
		}

		default:
			t = expr_to_stmt(parse_expr());

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

	decls = parse_decls(1, 0);
	EAT(token_eof);

	if(decls)
		for(i = 0; decls[i]; i++)
			symtab_add(globals, decls[i], sym_global, SYMTAB_NO_SYM, SYMTAB_APPEND);

	return globals;
}
