#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "tokenise.h"
#include "../util/util.h"
#include "tree.h"
#include "parse.h"
#include "../util/alloc.h"
#include "tokconv.h"
#include "../util/util.h"
#include "sym.h"

#define NEED_TYPE_YES 1
#define NEED_TYPE_NO  0

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
#define parse_expr() parse_expr_comma()
#define parse_expr_funcallarg() parse_expr_if()
expr *parse_expr();
#define accept(tok) ((tok) == curtok ? (EAT(tok), 1) : 0)

enum decl_spel
{
	SPEL_REQ,
	SPEL_OPT,
	SPEL_NONE
};

extern enum token curtok;

tree  *parse_code(void);
expr **parse_funcargs(void);
decl  *parse_decl(type *type, enum decl_spel need_spel);
type  *parse_type(int can_default);

expr *parse_expr_binary_op(void); /* needed to limit [+-] parsing */
expr *parse_expr_array(void);

expr *parse_lone_identifier()
{
	expr *e = expr_new();

	if(curtok != token_identifier)
		EAT(token_identifier); /* raise error */

	e->spel = token_current_spel();
	EAT(token_identifier);
	e->type = expr_identifier;

	return e;
}

expr *parse_expr_unary_op()
{
	extern int currentval;
	expr *e;

	switch(curtok){
		case token_sizeof:
			EAT(token_sizeof);
			e = expr_new();
			e->type = expr_sizeof;
			if(curtok == token_identifier){
				e->spel = token_current_spel();
				EAT(token_identifier);
			}else if(curtok_is_type()){
				e->tree_type->type->primitive = curtok_to_type_primitive();
				EAT(curtok);
			}else{
				EAT(token_identifier); /* raise error */
			}
			return e;

		case token_integer:
		case token_character:
			e = expr_new_val(currentval);
			EAT(curtok);
			return e;

		case token_string:
			e = expr_new();
			e->type = expr_str;
			token_get_current_str(&e->val.s, &e->strl);
			e->ptr_safe = 1;
			EAT(token_string);
			return e;

		case token_open_paren:
			EAT(token_open_paren);

			if(curtok_is_type_prething()){
				e = expr_new();
				e->type = expr_cast;
				e->lhs = expr_new();
				decl_free(e->lhs->tree_type);
				e->lhs->tree_type = parse_decl(NULL, SPEL_NONE);

				EAT(token_close_paren);
				e->rhs = parse_expr_array(); /* grab only the closest */
			}else{
				e = parse_expr();
				EAT(token_close_paren);
			}

			return e;

		case token_multiply: /* deref! */
			EAT(token_multiply);
			e = expr_new();
			e->type = expr_op;
			e->op   = op_deref;
			e->lhs  = parse_expr_unary_op();
			return e;

		case token_and:
			EAT(token_and);
			e = parse_lone_identifier();
			e->type = expr_addr;
			return e;

		case token_plus:
			EAT(token_plus);
			return parse_expr();

		case token_minus:
			e = expr_new();
			e->type = expr_op;
			e->op = curtok_to_op();

			EAT(token_minus);
			e->lhs = parse_expr_binary_op();
			return e;

		case token_not:
		case token_bnot:
			e = expr_new();
			e->type = expr_op;
			e->op = curtok_to_op();

			EAT(curtok);
			e->lhs = parse_expr_binary_op();
			return e;

		case token_increment:
		case token_decrement:
			e = expr_new();
			e->type = expr_assign;
			e->assign_type = curtok == token_increment ? assign_pre_increment : assign_pre_decrement;
			EAT(curtok);
			e->expr = parse_expr_unary_op();
			return e;

		case token_identifier:
			e = parse_lone_identifier();

			if(accept(token_open_paren)){
				e->type = expr_funcall;
				e->funcargs = parse_funcargs();
				EAT(token_close_paren);
			}else{
				int flag = 0;

				if((flag = accept(token_increment)) || accept(token_decrement)){
					expr *inc = expr_new();
					inc->type = expr_assign;
					inc->assign_type = flag ? assign_post_increment : assign_post_decrement;
					inc->expr = e;
					return inc;
				}
			}
			return e;

		default:
			die_at(NULL, "expected: unary expression, got %s", token_to_str(curtok));
	}
	return NULL;
}

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

		EAT(curtok);
		join->rhs = this();

		return join;
	}else{
		va_end(l);
		return e;
	}
}

expr *parse_expr_array()
{
	expr *sum, *deref;
	expr *base = parse_expr_unary_op();

	if(!accept(token_open_square))
		return base;

	sum = expr_new();

	sum->type = expr_op;
	sum->op   = op_plus;

	sum->lhs  = base;
	sum->rhs  = parse_expr();

	EAT(token_close_square);

	deref = expr_new();
	deref->type = expr_op;
	deref->op   = op_deref;
	deref->lhs  = sum;

	return deref;
}

expr *parse_expr_binary_op()
{
	/* above [/%*] above */
	return parse_expr_join(
			parse_expr_array, parse_expr_binary_op,
				token_multiply, token_divide, token_modulus,
				token_unknown);
}

expr *parse_expr_sum()
{
	/* above [+-] above */
	return parse_expr_join(
			parse_expr_binary_op, parse_expr_sum,
				token_plus, token_minus, token_unknown);
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

expr *parse_expr_assign()
{
	expr *e;

	e = parse_expr_logical_op();

	if(accept(token_assign)){
		e = expr_assignment(e, parse_expr_assign());
	}else if(curtok_is_augmented_assignment()){
		/* +=, ... */
		expr *ass = expr_new();

		ass->type = expr_assign;
		ass->assign_type = assign_augmented;

		ass->op = curtok_to_augmented_op();

		EAT(curtok);

		ass->lhs = e;
		ass->rhs = parse_expr();

		e = ass;
	}

	return e;
}

expr *parse_expr_if()
{
	expr *e = parse_expr_assign();
	if(accept(token_question)){
		expr *q = expr_new();

		q->type = expr_if;
		q->expr = e;
		if(accept(token_colon)){
			q->lhs = NULL; /* sentinel */
		}else{
			q->lhs = parse_expr();
			EAT(token_colon);
		}
		q->rhs = parse_expr();

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
		expr *ret = expr_new();
		ret->type = expr_comma;
		ret->lhs = e;
		ret->rhs = parse_expr_comma();
		return ret;
	}
	return e;
}

tree *parse_if()
{
	tree *t = tree_new();
	EAT(token_if);
	EAT(token_open_paren);

	t->type = stat_if;

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
		dynarray_add((void ***)&args, parse_expr_funcallarg());

		if(curtok == token_close_paren)
			break;
		EAT(token_comma);
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

tree *parse_switch(){return NULL;}

tree *parse_do()
{
	tree *t = tree_new();

	EAT(token_do);

	t->lhs = parse_code();

	EAT(token_while);
	EAT(token_open_paren);
	t->expr = parse_expr();
	EAT(token_close_paren);
	EAT(token_semicolon);

	t->type = stat_do;

	return t;
}

tree *parse_while()
{
	tree *t = tree_new();

	EAT(token_while);
	EAT(token_open_paren);

	t->expr = parse_expr();
	EAT(token_close_paren);
	t->lhs = parse_code();

	t->type = stat_while;

	return t;
}

tree *parse_for()
{
	tree *t = tree_new();
	tree_flow *tf;

	EAT(token_for);
	EAT(token_open_paren);

	tf = t->flow = tree_flow_new();

	tf->for_init  = parse_expr();
	EAT(token_semicolon);
	tf->for_while = parse_expr();
	EAT(token_semicolon);
	tf->for_inc   = parse_expr();
	EAT(token_close_paren);

	t->lhs = parse_code();

	t->type = stat_for;

	return t;
}

type *parse_type(int can_default)
{
	type *t = type_new();

	while(curtok_is_type_specifier()){
		const enum type_spec spec = curtok_to_type_specifier();

		if(t->spec & spec)
			die_at(NULL, "duplicate type specifier \"%s\"", spec_to_str(spec));

		t->spec |= spec;
		EAT(curtok);
	}

	if((t->primitive = curtok_to_type_primitive()) == type_unknown){
		if(!can_default && t->spec == spec_none)
			return NULL;

		warn_at(&t->where, "defaulting type to int");
		t->primitive = type_int; /* default to int */
	}else{
		EAT(curtok);
	}

	return t;
}

decl **parse_decls(const int need_type)
{
	decl **ret = NULL;

	for(; curtok != token_eof;){
		type *curtype = parse_type(!need_type); /* specific to each line */
		decl *d;

		if(need_type && !curtype)
			return ret;

next_decl:
		dynarray_add((void ***)&ret, d = parse_decl(curtype, SPEL_REQ));

		if(accept(token_open_paren)){
			d->func = function_new();

			/*
			 * either:
			 *
			 * [<type>] <name>( [<type> [<name>]]... )
			 * {
			 * }
			 *
			 * with optional {}
			 *
			 * or
			 *
			 * [<type>] <name>( [<name>...] )
			 *   <type> <name>, ...;
			 *   <type> <name>, ...;
			 * {
			 * }
			 *
			 * non-optional code
			 *
			 * i.e.
			 *
			 * int x(int y);
			 * int x(int y){}
			 *
			 * or
			 *
			 * int x(y)
			 *   int y;
			 * {
			 * }
			 *
			 */

			if(curtok == token_identifier){
				int i, len;
				char **spells = NULL;
				decl **args;

				do{
					dynarray_add((void ***)&spells, token_current_spel());
					EAT(token_identifier);
				}while(accept(token_comma));
				EAT(token_close_paren);

				/* parse decls, then check they correspond */
				args = parse_decls(NEED_TYPE_YES);

				len = dynarray_count((void ***)&args);
				if(len != dynarray_count((void ***)&spells))
					die_at(&args[0]->where, "mismatching argument counts");

				for(i = 0; i < len; i++){
					int j, found;

					if(args[i]->init)
						die_at(&args[i]->where, "parameter \"%s\" is initialised", args[i]->spel);

					found = 0;
					for(j = 0; j < len; j++)
						if(!strcmp(spells[j], args[i]->spel)){
							found = 1;
							break;
						}

					if(!found)
						die_at(&args[i]->where, "\"%s\": no corresponding identifier for \"%s\"", d->spel, args[i]->spel);
				}
				/* no need to check the other way around, since the counts are equal */
				/* FIXME: uniq() on parse_decls() in general */
				dynarray_free((void ***)&spells, free);

				d->func->args = args;
				d->func->code = parse_code();

			}else{
				while((curtok_is_type_prething())){
					/* actually, we don't need a type here, default to int, i think */
					dynarray_add((void ***)&d->func->args, parse_decl(parse_type(!need_type), SPEL_OPT));

					if(curtok == token_close_paren)
						break;

					EAT(token_comma);

					if(accept(token_elipsis)){
						d->func->variadic = 1;
						break;
					}

					/* continue loop */
				}

				EAT(token_close_paren);

				if(!accept(token_semicolon))
					d->func->code = parse_code();
			}
			/* end of function parsing */

		}else{
			if(accept(token_assign))
				d->init = parse_expr();

			if(accept(token_comma))
				/* should probably not accept functions as part of next decl */
				goto next_decl; /* don't read another type */

			EAT(token_semicolon);
		}
	}

	return ret;
}

tree *parse_code_declblock()
{
	tree *t = tree_new_code();
	decl **diter;

	EAT(token_open_block);

	if(accept(token_close_block))
		/* if(x){} */
		return t;

	t->decls = parse_decls(NEED_TYPE_YES);

	for(diter = t->decls; diter && *diter; diter++)
		if((*diter)->init){
			expr *sym = expr_new();

			sym = expr_new();
			sym->type = expr_identifier;
			sym->spel = (*diter)->spel;

			dynarray_add((void ***)&t->codes, expr_to_tree(expr_assignment(sym, (*diter)->init)));

			(*diter)->init = NULL; /* we are a code block, not global, this is fine */
		}

	/* main read loop */
	do{
		tree *sub = parse_code();

		if(sub)
			dynarray_add((void ***)&t->codes, sub);
		else
			break;
	}while(curtok != token_close_block);

	EAT(token_close_block);
	return t;
}

tree *parse_code()
{
	tree *t;

	switch(curtok){
		case token_semicolon:
			t = tree_new();
			t->type = stat_noop;
			EAT(token_semicolon);
			return t;

		case token_break:
		case token_return:
		case token_goto:
			t = tree_new();
			if(accept(token_break)){
				t->type = stat_break;
			}else if(accept(token_return)){
				t->type = stat_return;
				if(curtok != token_semicolon)
					t->expr = parse_expr();
			}else{
				EAT(token_goto);

				t->expr = parse_lone_identifier();
				t->type = stat_goto;
			}
			EAT(token_semicolon);
			return t;

		case token_switch: return parse_switch();
		case token_if:     return parse_if();
		case token_while:  return parse_while();
		case token_do:     return parse_do();
		case token_for:    return parse_for();

		case token_open_block: return parse_code_declblock();

		default:
			/* read an expression and optionally an assignment (fold checks for lvalues) */
			t = expr_to_tree(parse_expr());

			EAT(token_semicolon);
			return t;
	}

	/* unreachable */
}

decl *parse_decl(type *type, enum decl_spel need_spel)
{
	decl *d = decl_new();

	if(!type || type->primitive == type_unknown){
		type_free(type);
		d->type = parse_type(0);
	}else{
		d->type = type;
	}

	while(curtok == token_multiply){
		EAT(token_multiply);
		d->ptr_depth++;
	}

	if(curtok == token_identifier){
		if(need_spel == SPEL_NONE)
			die_at(NULL, "identifier not wanted here");
		d->spel = token_current_spel();
		EAT(token_identifier);
	}else if(need_spel == SPEL_REQ){
		die_at(NULL, "need identifier, not just type (%s)", type_to_str(d->type));
	}

	/* array parsing */
	while(curtok == token_open_square){
		expr *size;
		int fin;

		fin = 0;

		EAT(token_open_square);
		if(curtok != token_close_square)
			size = parse_expr(); /* fold->c checks for const-ness */
		else
			fin = 1;

		d->ptr_depth++;
		EAT(token_close_square);

		if(fin)
			break;

		dynarray_add((void ***)&d->arraysizes, size);
	}

	return d;
}

symtable *parse()
{
	symtable *globals = symtab_new();
	decl **decls = parse_decls(NEED_TYPE_NO);
	int i;

	for(i = 0; decls[i]; i++){
		symtab_add(globals, decls[i], sym_global);
		sym_free(decls[i]->sym);
		decls[i]->sym = NULL; /* fold links up */
	}

	EAT(token_eof);

	return globals;
}
