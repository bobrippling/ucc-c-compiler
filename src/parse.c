#include <stdio.h>
#include <stdarg.h>

#include "tokenise.h"
#include "tree.h"
#include "parse.h"
#include "alloc.h"
#include "tokconv.h"
#include "util.h"


/*
	* order goes:
	*
	* parse_expr_unary_op   [-+!~]`parse_expr_single`
	* parse_expr_binary_op  above [/%*]    above
	* parse_expr_sum        above [+-]     above
	* parse_expr_bit_op     above [&|]     above
	* parse_expr_cmp_op     above [><==!=] above
	* parse_expr_logical_op above [&&||]   above
	*/
#define parse_expr() parse_expr_logical_op()

extern enum token curtok;

tree  *parse_code();
expr **parse_funcargs();
expr  *parse_expr();
decl  *parse_decl(enum type type, int need_spel);


/* generalised recursive descent */
expr *parse_expr_join(
		expr *(*above)(), expr *(*this)(),
		enum token accept, ...
		)
{
	va_list l;
	expr *e = above();

	va_start(l, accept);
	if(curtok == accept || curtok_in_list(l)){
		expr *join;

		va_end(l);

		join       = expr_new();
		join->type = expr_op;
		join->op   = curtok_to_op();
		join->lhs  = e;

		eat(curtok);
		join->rhs = this();

		return join;
	}else{
		va_end(l);
		return e;
	}
}

expr *parse_expr_unary_op()
{
	extern int currentval;
	expr *e;

	switch(curtok){
		case token_sizeof:
			eat(token_sizeof);
			e = expr_new();
			e->type = expr_sizeof;
			if(curtok != token_identifier)
				eat(token_identifier); /* raise error */
			e->spel = token_current_spel();
			eat(token_identifier);
			return e;

		case token_integer:
		case token_character:
			e = expr_new();
			e->type = expr_val;
			e->val = currentval;
			eat(curtok);
			return e;

		case token_string:
			e = expr_new();
			e->type = expr_str;
			token_get_current_str(&e->spel, &e->val);
			eat(token_string);
			return e;

		case token_open_paren:
			eat(token_open_paren);
			e = parse_expr();
			eat(token_close_paren);
			return e;

		case token_multiply:
			eat(token_multiply);
			e = expr_new();
			e->type = expr_op;
			e->op   = op_deref;
			e->lhs  = parse_expr();
			return e;

		case token_and:
			eat(token_and);
			if(curtok != token_identifier)
				eat(token_identifier); /* raise error */
			e = expr_new();
			e->type = expr_addr;
			e->spel = token_current_spel();
			eat(token_identifier);
			return e;

		case token_plus:
			eat(token_plus);
			return parse_expr();

		case token_minus:
		case token_not:
		case token_bnot:
			e = expr_new();
			e->type = expr_op;
			e->op = curtok_to_op();

			eat(curtok);
			e->lhs = parse_expr_unary_op();
			return e;

		case token_increment:
		case token_decrement:
			fprintf(stderr, "TODO: ");
			break;

		case token_identifier:
			e = expr_new();

			e->spel = token_current_spel();
			eat(token_identifier);

			if(curtok == token_assign){
				eat(token_assign);

				e->type = expr_assign;
				e->expr = parse_expr();
				return e;

			}else if(curtok == token_open_paren){
				eat(token_open_paren);
				e->type = expr_funcall;
				e->funcargs = parse_funcargs();
				eat(token_close_paren);
				return e;
			}else{
				e->type = expr_identifier;
				return e;
			}

		default:
			break;
	}
	fprintf(stderr, "warning: parse_expr_unary_op() returning NULL @ %s\n", token_to_str(curtok));
	return NULL;
}

expr *parse_expr_binary_op()
{
	/* above [/%*] above */
	return parse_expr_join(
			parse_expr_unary_op, parse_expr_binary_op,
				token_divide, token_multiply, token_modulus,
				token_unknown);
}

expr *parse_expr_sum()
{
	/* above [+] above (token_minus is handled as a unary operator) */
	return parse_expr_join(
			parse_expr_binary_op, parse_expr_sum,
				token_plus, token_unknown);
}

expr *parse_expr_bit_op()
{
	/* above [&|] above */
	return parse_expr_join(
			parse_expr_sum, parse_expr_bit_op,
				token_and, token_or, token_unknown);
}

expr *parse_expr_cmp_op()
{
	/* above [><==!=] above */
	return parse_expr_join(
			parse_expr_bit_op, parse_expr_cmp_op,
				token_eq, token_ne,
				token_le, token_lt,
				token_ge, token_gt,
				token_unknown);
}

expr *parse_expr_logical_op()
{
	/* above [&&||] above */
	return parse_expr_join(
			parse_expr_cmp_op, parse_expr_logical_op,
			token_orsc, token_andsc, token_unknown);
}

tree *parse_if()
{
	tree *t = tree_new();
	eat(token_if);
	eat(token_open_paren);

	t->type = stat_if;

	t->expr = parse_expr();

	eat(token_close_paren);

	t->lhs = parse_code();

	if(curtok == token_else){
		eat(token_else);
		t->rhs = parse_code();
	}

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
	tree *t = tree_new();
	t->type = stat_expr;
	t->expr = e;
	return t;
}

tree*parse_switch(){return NULL;}
tree*parse_do(){return NULL;}

tree *parse_while()
{
	tree *t = tree_new();

	eat(token_while);
	eat(token_open_paren);

	t->expr = parse_expr();
	eat(token_close_paren);
	t->lhs = parse_code();

	t->type = stat_while;

	return t;
}

tree *parse_for()
{
	tree *t = tree_new();
	tree_flow *tf;

	eat(token_for);
	eat(token_open_paren);

	tf = t->flow = tree_flow_new();

	tf->for_init  = parse_expr();
	eat(token_semicolon);
	tf->for_while = parse_expr();
	eat(token_semicolon);
	tf->for_inc   = parse_expr();
	eat(token_close_paren);

	t->lhs = parse_code();

	t->type = stat_for;

	return t;
}

tree *parse_code_declblock()
{
	tree *t = tree_new();
	enum type curtype;

	t->type = stat_code;

	eat(token_open_block);

	while(curtok_is_type()){
		curtype = curtok_to_type();
		eat(curtok);
next_decl:
		dynarray_add((void ***)&t->decls, parse_decl(curtype, 1));

		if(curtok == token_comma){
			eat(token_comma);
			goto next_decl; /* don't read another type */
		}
		eat(token_semicolon);
	}

	/* main read loop */
	do{
		tree *sub = parse_code();

		if(sub)
			dynarray_add((void ***)&t->codes, sub);
		else
			break;
	}while(curtok != token_close_block);

	eat(token_close_block);
	return t;
}

tree *parse_code()
{
	tree *t;

	switch(curtok){
		case token_semicolon:
			t = tree_new();
			t->type = stat_noop;
			eat(token_semicolon);
			return t;


		case token_break:
		case token_return:
			t = tree_new();
			if(curtok == token_break){
				t->type = stat_break;
				eat(token_break);
			}else{
				t->type = stat_return;
				eat(token_return);
				if(curtok != token_semicolon)
					t->expr = parse_expr();
			}
			eat(token_semicolon);
			return t;

		case token_switch: return parse_switch();
		case token_if:     return parse_if();
		case token_while:  return parse_while();
		case token_do:     return parse_do();
		case token_for:    return parse_for();

		case token_open_block: return parse_code_declblock();

		default: break;
	}

	t = expr_to_tree(parse_expr());
	eat(token_semicolon);
	return t;
}

decl *parse_decl(enum type type, int need_spel)
{
	decl *d = decl_new();

	if(type == type_unknown){
		d->type = curtok_to_type();
		if(d->type == type_unknown){
			d->type = type_int; /* default to int */
		}else{
			eat(curtok);
		}
	}else{
		d->type = type;
	}

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
	function *f = function_new();

	f->func_decl = parse_decl(type_unknown, 1);
	f->func_decl->func = 1;

	eat(token_open_paren);

	while((curtok_is_type())){
		dynarray_add((void ***)&f->args, parse_decl(type_unknown, 0));

		if(curtok == token_close_paren)
			break;

		eat(token_comma);
		/* continue loop */
	}

	eat(token_close_paren);

	return f;
}

function *parse_function()
{
	function *f = parse_function_proto();

	if(curtok == token_semicolon)
		eat(token_semicolon);
	else
		f->code = parse_code();

	return f;
}

function **parse()
{
	function **f;
	int i, n;

	n = 10;
	i = 0;
	f = umalloc(n * sizeof *f);

	do{
		f[i++] = parse_function();

		if(i == n)
			f = urealloc(f, (n += 10) * sizeof *f);
	}while(curtok != token_eof);

	return f;
}
