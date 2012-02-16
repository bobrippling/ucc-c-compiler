#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "../util/util.h"
#include "tree.h"
#include "tokenise.h"
#include "parse.h"
#include "../util/alloc.h"
#include "tokconv.h"
#include "../util/util.h"
#include "sym.h"
#include "cc1.h"
#include "typedef.h"
#include "../util/dynarray.h"
#include "struct.h"
#include "parse_type.h"


/*
 * order goes:
 *
 * parse_expr_unary_op   [-+!~]`parse_expr_single`
 * parse_expr_binary_op  above [/%*]    above
 * parse_expr_sum        above [+-]     above
 * parse_expr_shift      above [<<|>>]  above
 * parse_expr_bit_op     above [&|]     above
 * parse_expr_cmp_op     above [><==!=] above
 * parse_expr_logical_op above [&&||]   above
 */

tdeftable *typedefs_current;
struc    **structs_current;

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
	expr *e;

	switch(curtok){
		case token_sizeof:
			EAT(token_sizeof);
			e = expr_new();
			e->type = expr_sizeof;

			if(accept(token_open_paren)){
				decl *d = parse_decl_single(DECL_SPEL_NO);

				if(d){
					e->expr = expr_new();
					e->expr->type = expr_cast;
					decl_free(e->expr->tree_type);
					e->expr->tree_type = d;
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

		case token_string:
		case token_open_block: /* array */
			e = expr_new();
			e->array_store = array_decl_new();

			e->type = expr_addr;
			/*e->ptr_safe = 1;*/

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
				e = expr_new();
				e->type = expr_cast;
				e->lhs = expr_new();
				decl_free(e->lhs->tree_type);
				e->lhs->tree_type = d;

				EAT(token_close_paren);
				e->rhs = parse_expr_array(); /* grab only the closest */
			}else{
				e = parse_expr();
				EAT(token_close_paren);
			}

			return e;
		}

		case token_and:
			EAT(token_and);
			e = parse_lone_identifier();
			e->type = expr_addr;
			if(accept(token_open_square)){
				/* &x[5] */
				expr *new = expr_new();
				new->lhs = e;
				new->rhs = parse_expr();
				EAT(token_close_square);

				new->type = expr_op;
				new->op   = op_plus;

				return new;
			}
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
		{
			const int inc = curtok == token_increment;
			/* this is a normal increment, i.e. ++x, simply translate it to x = x + 1 */
			e = expr_new();
			e->type = expr_assign;
			EAT(curtok);

			/* assign to... */
			e->lhs = parse_expr_deref();
			e->rhs = expr_new();
			e->rhs->op = inc ? op_plus : op_minus;
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

			join       = expr_new();
			join->type = expr_op;
			join->op   = curtok_to_op();
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

expr *parse_expr_struct()
{
	return parse_expr_join(parse_expr_unary_op,
				token_ptr, token_dot,
				token_unknown);
}

expr *parse_expr_array()
{
	expr *base = parse_expr_struct();

	while(accept(token_open_square)){
		expr *sum, *deref;

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


		base = deref;
	}

	return base;
}

expr *parse_expr_inc_dec()
{
	expr *e = parse_expr_array();
	int flag = 0;

	if((flag = accept(token_increment)) || accept(token_decrement)){
		expr *inc = expr_new();
		inc->type = expr_assign;
		inc->assign_is_post = 1;

		inc->lhs = e;
		inc->rhs = expr_new();
		inc->rhs->op = flag ? op_plus : op_minus;
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
		e = expr_new();
		e->type = expr_funcall;
		e->funcargs = parse_funcargs();
		e->expr = sub;
		EAT(token_close_paren);
	}

	return e;
}

expr *parse_expr_deref()
{
	if(accept(token_multiply)){
		expr *e = expr_new();
		e->type = expr_op;
		e->op   = op_deref;
		e->lhs  = parse_expr_deref();
		return e;
	}

	return parse_expr_funcall();
}


expr *parse_expr_binary_op()
{
	/* above [/%*] above */
	return parse_expr_join(parse_expr_deref,
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
	/* above [&|] above */
	return parse_expr_join(parse_expr_shift,
				token_and, token_or, token_unknown);
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

		ass->lhs = e;
		ass->rhs = expr_new();

		ass->rhs->op = curtok_to_augmented_op();
		EAT(curtok);

		ass->rhs->lhs = e;
		ass->rhs->rhs = parse_expr();

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

tree *parse_switch()
{
	tree *t = tree_new();

	EAT(token_switch);
	EAT(token_open_paren);

	t->type = stat_switch;
	t->expr = parse_expr();

	EAT(token_close_paren);

	t->lhs = parse_code();

	return t;
}

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

#define SEMI_WRAP(code) \
	if(!accept(token_semicolon)){ \
		code; \
		EAT(token_semicolon); \
	}

	SEMI_WRAP(tf->for_init  = parse_expr());
	SEMI_WRAP(tf->for_while = parse_expr());

#undef SEMI_WRAP

	if(!accept(token_close_paren)){
		tf->for_inc   = parse_expr();
		EAT(token_close_paren);
	}

	t->lhs = parse_code();

	t->type = stat_for;

	return t;
}

function *parse_function()
{
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
	function *f;
	decl *argdecl;

	f = function_new();

	if(accept(token_close_paren))
		goto empty_func;
	if(curtok == token_identifier && !TYPEDEF_FIND())
		goto old_func;

	argdecl = parse_decl_single(DECL_CAN_DEFAULT);

	if(argdecl){
		do{
			dynarray_add((void ***)&f->args, argdecl);

			if(curtok == token_close_paren)
				break;

			EAT(token_comma);

			if(accept(token_elipsis)){
				f->variadic = 1;
				break;
			}

			/* continue loop */
			/* actually, we don't need a type here, default to int, i think */
			argdecl = parse_decl_single(DECL_CAN_DEFAULT);
		}while(argdecl);

		EAT(token_close_paren);

		if(dynarray_count((void *)f->args) == 1 &&
				f->args[0]->type->primitive == type_void &&
				f->args[0]->ptr_depth == 0 &&
				f->args[0]->spel == NULL){
			/* x(void); */
			function_empty_args(f);
			f->args_void = 1; /* (void) vs () */
		}

empty_func:
		if(curtok != token_semicolon)
			f->code = parse_code();
		else
			EAT(token_semicolon);

	}else{
		int i, n_spels, n_decls;
		char **spells;
		decl **args;

old_func:
		spells = NULL;

		do{
			if(curtok != token_identifier)
				die_at(&f->where, "expected: identifier, got %s", token_to_str(curtok));

			dynarray_add((void ***)&spells, token_current_spel());
			EAT(token_identifier);

			if(accept(token_close_paren))
				break;
			EAT(token_comma);

		}while(1);

		/* parse decls, then check they correspond */
		args = PARSE_DECLS();

		n_decls = dynarray_count((void *)args);
		n_spels = dynarray_count((void *)spells);

		if(n_decls > n_spels)
			die_at(args ? &args[0]->where : &f->where, "old-style function decl: mismatching argument counts");

		for(i = 0; i < n_spels; i++){
			int j, found;

			found = 0;
			for(j = 0; j < n_decls; j++)
				if(!strcmp(spells[i], args[j]->spel)){
					if(args[j]->init)
						die_at(&args[j]->where, "parameter \"%s\" is initialised", args[j]->spel);

					found = 1;
					break;
				}

			if(!found){
				/*
					* void f(x){ ... }
					* - x is implicitly int
					*/
				decl *d = decl_new();
				d->type->primitive = type_int;
				d->spel = spells[i];
				spells[i] = NULL; /* prevent free */
				dynarray_add((void ***)&args, d);
			}
		}

		/* no need to check the other way around, since the counts are equal */
		if(spells)
			dynarray_free((void ***)&spells, free);

		f->args = args;

		if(curtok != token_open_block)
			die_at(&f->where, "no code for old-style function");
		f->code = parse_code();
	}

	return f;
}

tree *parse_code_block()
{
	tree *t = tree_new_code();
	decl **diter;

	EAT(token_open_block);

	if(accept(token_close_block))
		return t;

	t->decls = PARSE_DECLS();

	for(diter = t->decls; diter && *diter; diter++)
		/* only extract the init if it's not static */
		if((*diter)->init && ((*diter)->type->spec & spec_static) == 0){
			expr *e = expr_new();

			e = expr_new();
			e->type = expr_identifier;
			e->spel = (*diter)->spel;

			dynarray_add((void ***)&t->codes, expr_to_tree(expr_assignment(e, (*diter)->init)));

			/*
			 *(*diter)->init = NULL;
			 * leave it set, so we can check later in, say, fold.c for const init
			 */
		}

	if(curtok != token_close_block){
		/* main read loop */
		do{
			tree *sub = parse_code();

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
	return t;
}

tree *parse_label_next(tree *lbl)
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
			t = tree_new();
			t->type = stat_default;
			return parse_label_next(t);
		case token_case:
		{
			expr *a;
			EAT(token_case);
			a = parse_expr();
			if(accept(token_elipsis)){
				t = tree_new();
				t->type = stat_case_range;
				t->expr  = a;
				t->expr2 = parse_expr();
			}else{
				t = expr_to_tree(a);
				t->type = stat_case;
			}
			EAT(token_colon);
			return parse_label_next(t);
		}

		default:
			t = expr_to_tree(parse_expr());

			if(t->expr->type == expr_identifier && accept(token_colon)){
				t->type = stat_label;
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

	typedefs_current = umalloc(sizeof *typedefs_current);
	globals = symtab_new();

	decls = parse_decls(1, 0);
	EAT(token_eof);

	if(decls)
		for(i = 0; decls[i]; i++){
			symtab_add(globals, decls[i], sym_global, SYMTAB_NO_SYM, SYMTAB_APPEND);
			UCC_ASSERT(!decls[i]->sym, "symtab_add(... SYMTAB_NO_SYM) gave a sym");
		}

	globals->structs = structs_current; /* FIXME: structs should be per-block */

	EAT(token_eof);

	return globals;
}
