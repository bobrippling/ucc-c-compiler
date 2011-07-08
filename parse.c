#include <stdio.h>

#include "parse.h"
#include "tokenise.h"
#include "tree.h"
#include "alloc.h"
#include "tokconv.h"
#include "util.h"

extern enum token curtok;

tree *parse_code();
decl *parse_decl(int need_spel);

expr *parse_expr()
{
	extern int currentval;
	expr *e;

	switch(curtok){
		case token_sizeof:
			e = umalloc(sizeof *e);
			e->type = expr_const;
			e->const_type = const_sizeof;
			if(curtok != token_identifier)
				eat(token_identifier); /* raise error */
			e->spel = token_current_spel();
			eat(token_identifier);
			return e;

		case token_integer:
		case token_character:
			e = umalloc(sizeof *e);
			e->type = expr_const;
			e->const_type = const_val;
			e->val = currentval;
			eat(curtok);
			return e;

		case token_string:
			e = umalloc(sizeof *e);
			e->type = expr_const;
			e->const_type = const_str;
			e->spel = token_current_str();
			eat(token_string);
			return e;

		case token_identifier:
			e = umalloc(sizeof *e);
			e->type = expr_const;
			e->const_type = const_identifier;
			e->spel = token_current_spel();
			eat(token_identifier);
			return e;

		case token_open_paren:
			eat(token_open_paren);
			e = parse_expr();
			eat(token_close_paren);
			return e;

		case token_multiply:
			eat(token_multiply);
			e = umalloc(sizeof *e);
			e->type = expr_op;
			e->op = op_deref;
			e->lhs  = parse_expr();
			return e;

		case token_and:
			eat(token_and);
			if(curtok != token_identifier)
				eat(token_identifier); /* raise error */
			e = umalloc(sizeof *e);
			e->type = expr_const;
			e->const_type = const_addr;
			e->spel = token_current_spel();
			eat(token_identifier);
			return e;

		case token_plus:
			eat(token_plus);
			return parse_expr();

		case token_minus:
		case token_not:
		case token_bnot:
			e = umalloc(sizeof *e);
			e->type = expr_op;
			e->op = curtok_to_op();
			eat(curtok);

			if(curtok == token_minus){
				e->lhs = umalloc(sizeof *e->lhs);
				e->lhs->type = expr_const;
				e->lhs->const_type = const_val;
				e->lhs->val = 0; /* e.g. 0 - `expr` */

				e->rhs = parse_expr();
			}else{
				e->lhs = parse_expr();
			}
			break;

		default:
			break;
	}
	return NULL;
}

tree *parse_if()
{
	tree *t = umalloc(sizeof *t);
	eat(token_if);
	eat(token_open_paren);

	t->expr = parse_expr();

	eat(token_close_paren);

	t->lhs = parse_code();

	return t;
}

expr **parse_funcargs()
{
	expr **args = NULL;

	while(curtok != token_close_paren){
		dynarray_add((void ***)&args, parse_expr());

		if(curtok == token_close_paren)
			break;
		eat(token_comma);
	}

	return args;
}


tree *expr_to_tree(expr *e)
{
	tree *t = umalloc(sizeof *t);
	t->type = stat_expr;
	t->expr = e;
	return t;
}

tree *parse_identifier()
{
	/*
	 * expect:
	 * assignment
	 *   x = 5;
	 *   *y = 2;
	 *   *x[2] = 3;
	 *
	 * funcall
	 *   f(x);
	 */

	tree *t = umalloc(sizeof *t);

	t->lhs = expr_to_tree(parse_expr());

	if(curtok == token_assign){
		/* expr - const_str, for e.g. */
		eat(token_assign);

		t->type = stat_assign;
		t->rhs = expr_to_tree(parse_expr());

	}else if(curtok == token_open_paren){
		eat(token_open_paren);
		t->type = stat_funcall;

		t->funcargs = parse_funcargs();
	}else{
		die_at("expected assignment or funcall");
	}

	return t;
}

tree*parse_switch(){return NULL;}
tree*parse_while(){return NULL;}
tree*parse_do(){return NULL;}
tree*parse_for(){return NULL;}

tree *parse_code_single()
{
	tree *t = umalloc(sizeof *t);

	switch(curtok){
		case token_semicolon: t->type = stat_noop;   break;
		case token_break:     t->type = stat_break;  eat(token_break);  break;
		case token_return:    t->type = stat_return; eat(token_return); break;

		case token_switch: return parse_switch();
		case token_if:     return parse_if();
		case token_while:  return parse_while();
		case token_do:     return parse_do();
		case token_for:    return parse_for();

		case token_identifier: return parse_identifier(); /* x = `expr` */

		default: break;
	}

	return NULL;
}

tree *parse_code()
{
	tree *t = umalloc(sizeof *t);

	if(curtok == token_open_block){
		eat(token_open_block);

		if(curtok_is_type()){
next_decl:
			dynarray_add((void ***)&t->decls, parse_decl(1));

			if(curtok == token_comma){
				eat(token_comma);
				goto next_decl;
			}
			eat(token_semicolon);
		}

		/* main read loop */
		do{
			tree *sub = parse_code_single();

			eat(token_semicolon);

			if(sub)
				dynarray_add((void ***)&t->lhs, sub);
			else
				break;
		}while(curtok != token_close_block);

		eat(token_close_block);
	}

	return t;
}

decl *parse_decl(int need_spel)
{
	decl *d = umalloc(sizeof *d);

	d->type = curtok_to_type();
	eat(curtok);

	while(curtok == token_multiply){
		eat(token_multiply);
		d->ptr_depth++;
	}

	if(curtok == token_identifier){
		d->spel = token_current_spel();
		eat(token_identifier);
	}else if(need_spel){
		eat(token_identifier); /* raise error */
	}

	return d;
}

function *parse_function_proto()
{
	function *f = umalloc(sizeof *f);

	f->func_decl = parse_decl(1);

	eat(token_open_paren);

	while((curtok_is_type())){
		decl *d = parse_decl(0);

		dynarray_add((void ***)&f->args, d);

		if(curtok == token_close_paren){
			eat(token_close_paren);
			break;
		}

		eat(token_comma);
		/* continue loop */
	}

	eat(token_close_paren);

	return f;
}

function *parse_function()
{
	function *f = parse_function_proto();

	if(curtok == token_semicolon){
		eat(token_semicolon);
	}else{
		f->code = parse_code();
	}

	return f;
}

void parse()
{
	function **f;
	int i, n;

	n = 10;
	i = 0;
	f = umalloc(n * sizeof *f);

	do{
		f[i++] = parse_function();

		if(i == n){
			n += 10;
			f = urealloc(f, n * sizeof *f);
		}
	}while(curtok != token_eof);
}
