#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>

#include "../util/dynarray.h"
#include "../util/util.h"
#include "../util/alloc.h"

#include "cc1_where.h"
#include "cc1.h" /* cc1_std */
#include "str.h"

#include "tokenise.h"
#include "tokconv.h"

#include "parse_stmt.h"

#include "parse_type.h"
#include "parse_expr.h"

#include "fold.h"

#include "ops/expr_identifier.h"

struct stmt_ctx
{
	stmt *continue_target,
	     *break_target,
	     *switch_target;
	symtable *scope;

	int parsing_unbraced_if; /* dangling-else detection */
};

static void parse_test_init_expr(stmt *t, struct stmt_ctx *ctx)
{
	where here;

	where_cc1_current(&here);

	EAT(token_open_paren);

	/* if C99, we create a new scope here, for e.g.
	 * if(5 > (enum { a, b })a){ return a; } return b;
	 * "return b" can't see 'b' since its scope is only the if
	 *
	 * C90 drags the scope of the enum up to the enclosing block
	 */
	t->symtab = (cc1_std >= STD_C99 ? symtab_new : symtab_new_transparent)(
				t->symtab, &here);

	ctx->scope = t->symtab;

  if(parse_at_decl(ctx->scope, 1)){
		decl *d;

		/* if we are at a type, push a scope for it, for
		 * for(int i ...), if(int i = ...) etc
		 */
		symtable *init_scope = symtab_new(t->symtab, &here);

		t->flow = stmt_flow_new(init_scope);

		d = parse_decl(
				DECL_SPEL_NEED, 0,
				init_scope, init_scope);

		UCC_ASSERT(d, "at decl, but no decl?");

		UCC_ASSERT(
				t->flow->for_init_symtab == init_scope,
				"wrong scope for stmt-init");

		flow_fold(t->flow, &t->symtab);
		ctx->scope = t->symtab;

		/* `d' is added to the scope implicitly */

		if(accept(token_semicolon)){
			/* if(int i = 5; i > f()){ ... } */
			t->expr = parse_expr_exp(ctx->scope, 0);
		}else{
			/* if(int i = 5) -> if(i) */
			t->expr = expr_set_where(
					expr_new_identifier(d->spel),
					&d->where);
		}
	}else{
		t->expr = parse_expr_exp(t->symtab, 0);
	}
	FOLD_EXPR(t->expr, t->symtab);

	EAT(token_close_paren);
}

static stmt *parse_if(const struct stmt_ctx *const ctx)
{
	stmt *t = stmt_new_wrapper(if, ctx->scope);
	struct stmt_ctx subctx = *ctx;
	where else_locn;

	EAT(token_if);

	subctx.parsing_unbraced_if = (curtok != token_open_block);

	parse_test_init_expr(t, &subctx);

	t->lhs = parse_stmt(&subctx);

	if(accept_where(token_else, &else_locn)){
		if(ctx->parsing_unbraced_if){
			/* if we are already parsing an if-statement that hasn't got braces and run
			 * into an else while pasing an inner-if statement, then we have a dangling
			 * else
			 */
			cc1_warn_at(&else_locn, dangling_else, "dangling else statement");
		}

		subctx.parsing_unbraced_if = 0; /* parsing the else, can't be ambiguous now */
		t->rhs = parse_stmt(&subctx);
	}

	return t;
}

static stmt *parse_switch(const struct stmt_ctx *const ctx)
{
	stmt *t = stmt_new_wrapper(switch, ctx->scope);
	struct stmt_ctx subctx = *ctx;

	subctx.break_target = t;
	subctx.switch_target = t;

	EAT(token_switch);

	parse_test_init_expr(t, &subctx);

	t->lhs = parse_stmt(&subctx);

	return t;
}

static stmt *parse_do(const struct stmt_ctx *const ctx)
{
	stmt *t = stmt_new_wrapper(do, ctx->scope);
	struct stmt_ctx subctx = *ctx;

	subctx.break_target = t;
	subctx.continue_target = t;

	EAT(token_do);

	t->lhs = parse_stmt(&subctx);

	EAT(token_while);
	EAT(token_open_paren);
	t->expr = parse_expr_exp(subctx.scope, 0);
	fold_expr_nodecay(t->expr, ctx->scope);
	EAT(token_close_paren);
	EAT(token_semicolon);

	return t;
}

static stmt *parse_while(const struct stmt_ctx *const ctx)
{
	stmt *t = stmt_new_wrapper(while, ctx->scope);
	struct stmt_ctx subctx = *ctx;

	subctx.continue_target = t;
	subctx.break_target = t;

	EAT(token_while);

	parse_test_init_expr(t, &subctx);

	t->lhs = parse_stmt(&subctx);

	return t;
}

static stmt *parse_for(const struct stmt_ctx *const ctx)
{
	stmt *s = stmt_new_wrapper(for, ctx->scope);
	stmt_flow *sf;
	struct stmt_ctx subctx = *ctx;

	subctx.continue_target = s;
	subctx.break_target = s;

	EAT(token_for);
	EAT(token_open_paren);

	sf = s->flow = stmt_flow_new(symtab_new(s->symtab, &s->where));

	subctx.scope = sf->for_init_symtab;

	if(!accept(token_semicolon)){
		int got_decls;
		where w;

		where_cc1_current(&w);

		got_decls = parse_decl_group(
				DECL_MULTI_ALLOW_ALIGNAS | DECL_MULTI_ALLOW_STORE,
				/*newdecl context:*/1,
				subctx.scope, subctx.scope,
				NULL);

		if(got_decls){
			if(cc1_std < STD_C99)
				cc1_warn_at(&w, c89_for_init, "use of C99 for-init");

			stmt_for_got_decls(s);
		}else{
			sf->for_init = parse_expr_exp(subctx.scope, 0);

			flow_fold(s->flow, &s->symtab);
			subctx.scope = s->symtab;

			FOLD_EXPR(sf->for_init, subctx.scope);
			EAT(token_semicolon);
		}

		/* ';' eaten by parse_decls_single_type() */
	}

	if(!accept(token_semicolon)){
		sf->for_while = parse_expr_exp(subctx.scope, 0);
		FOLD_EXPR(sf->for_while, subctx.scope);
		EAT(token_semicolon);
	}

	if(!accept(token_close_paren)){
		sf->for_inc = parse_expr_exp(subctx.scope, 0);
		FOLD_EXPR(sf->for_inc, subctx.scope);
		EAT(token_close_paren);
	}

	s->lhs = parse_stmt(&subctx);

	return s;
}

where *parse_static_assert(symtable *scope)
{
	where *found = NULL;

	while(accept(token__Static_assert)){
		static_assert *sa = umalloc(sizeof *sa);

		sa->scope = scope;

		EAT(token_open_paren);
		sa->e = PARSE_EXPR_NO_COMMA(scope, 0);

		if(accept(token_comma)){
			struct cstring *str = parse_asciz_str();
			if(!str){
				expr_free(sa->e);
				free(sa);
				continue;
			}
			sa->s = cstring_detach(str);
			EAT(token_string);
		}else{
			sa->s = NULL;
			if(cc1_std < STD_C2X){
				cc1_warn_at(&sa->e->where, static_assert_missing_string,
						"omitting the string argument of _Static_assert is a C2X extension");
			}
		}

		EAT(token_close_paren);
		EAT(token_semicolon);

		dynarray_add(&scope->static_asserts, sa);
		found = &sa->e->where;
	}
	return found;
}

static stmt *parse_label_next(stmt *lbl, const struct stmt_ctx *ctx)
{
	lbl->lhs = parse_stmt(ctx);
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

static stmt *parse_label(const struct stmt_ctx *ctx)
{
	where w;
	char *lbl;
	attribute **attr = NULL, **ai;
	stmt *lblstmt;

	where_cc1_current(&w);
	lbl = token_eat_identifier("?", &w);

	EAT(token_colon);

	lblstmt = stmt_new_wrapper(label, ctx->scope);
	lblstmt->bits.lbl.spel = lbl;
	memcpy_safe(&lblstmt->where, &w);

	parse_add_attr(&attr, ctx->scope);

	for(ai = attr; ai && *ai; ai++)
		if((*ai)->type == attr_unused)
			lblstmt->bits.lbl.unused = 1;
		else
			cc1_warn_at(&(*ai)->where,
					lbl_attr_unknown,
					"ignoring attribute \"%s\" on label",
					attribute_to_str(*ai));

	attribute_array_release(&attr);

	return parse_label_next(lblstmt, ctx);
}

static void parse_local_labels(const struct stmt_ctx *const ctx)
{
	while(accept(token___label__)){
		for(;;){
			char *spel;
			where loc;
			int created;

			spel = token_eat_identifier(NULL, &loc);
			if(!spel)
				break;

			created = symtab_label_add_local(ctx->scope, spel, &loc);

			if(!created){
				warn_at_print_error(&loc, "local label \"%s\" already defined", spel);
				fold_had_error = 1;
				free(spel);
			}

			if(accept(token_semicolon))
				break;
			EAT(token_comma);
		}
	}
}

static void apply_stmt_attrs(stmt *s, attribute ***const attrs)
{
	attribute **i;
	int have_fallthrough = 0;

	if(!attrs)
		return;

	for(i = *attrs; i && *i; i++){
		attribute *attr = *i;
		if(attr->type == attr_fallthrough)
			have_fallthrough = 1;
		else
			cc1_warn_at(&attr->where, attr_ignored, "non-fallthrough attribute on statement");
	}
	attribute_array_release(attrs);
	*attrs = NULL;

	if(!have_fallthrough)
		return;

	if(!stmt_kind(s, noop)){
		warn_at_print_error(
				&s->where,
				"fallthrough attribute on %s statement",
				s->f_str());
		fold_had_error = 1;
		return;
	}

	s->bits.noop.is_fallthrough = 1;

	switch(curtok){
		case token_case:
		case token_default:
			break;
		default:
			if(tok_at_label())
				break;

			warn_at_print_error(&s->where, "fallthrough statement not directly before switch label");
			fold_had_error = 1;
			break;
	}
}

ucc_nonnull()
static stmt *parse_stmt_with_attrs(const struct stmt_ctx *ctx, attribute ***attrs)
{
	stmt *t;

	parse_add_attr(attrs, ctx->scope);

	switch(curtok){
		case token_semicolon:
			t = stmt_new_wrapper(noop, ctx->scope);
			EAT(token_semicolon);
			break;

		case token_break:
		case token_continue:
		case token_goto:
		case token_return:
		{
			if(accept(token_break)){
				t = stmt_new_wrapper(break, ctx->scope);
				t->parent = ctx->break_target;

			}else if(accept(token_continue)){
				t = stmt_new_wrapper(continue, ctx->scope);
				t->parent = ctx->continue_target;

			}else if(accept(token_return)){
				t = stmt_new_wrapper(return, ctx->scope);

				if(curtok != token_semicolon){
					t->expr = parse_expr_exp(ctx->scope, 0);
					fold_expr_nodecay(t->expr, ctx->scope);
				}
			}else{
				t = stmt_new_wrapper(goto, ctx->scope);

				EAT(token_goto);

				if(accept(token_multiply)){
					/* computed goto */
					t->expr = parse_expr_exp(ctx->scope, 0);
				}else{
					char *spel;
					if(token_accept_identifier(&spel, NULL)){
						assert(spel);
					}else{
						assert(!spel);
						spel = ustrdup(DUMMY_IDENTIFIER);
						warn_at_print_error(NULL, "identifier or '*' expected for goto");
					}
					t->bits.lbl.spel = spel;
				}
			}
			EAT(token_semicolon);
			break;
		}

		{
			stmt *(*parse_f)(const struct stmt_ctx *);

		case token_if:
			parse_f = parse_if;
			goto flow;
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
			t = parse_f(ctx);
			break;
		}

		case token_open_block:
		{
			struct stmt_ctx subctx = *ctx;

			/* we're in a block - if we find an else in here, it's unambiguous */
			subctx.parsing_unbraced_if = 0;

			t = parse_stmt_block(ctx->scope, &subctx);
			break;
		}

		case token_switch:
			t = parse_switch(ctx);
			break;

		case token_default:
			t = stmt_new_wrapper(default, ctx->scope);
			EAT(token_default);
			EAT(token_colon);
			t->parent = ctx->switch_target;
			t = parse_label_next(t, ctx);
			break;

		case token_case:
		{
			expr *a;
			where cse_loc;
			where_cc1_current(&cse_loc);

			EAT(token_case);
			a = PARSE_EXPR_CONSTANT(ctx->scope, 0);
			if(accept(token_elipsis)){
				t = stmt_new_wrapper(case_range, ctx->scope);
				t->parent = ctx->switch_target;
				t->expr  = a;
				t->expr2 = PARSE_EXPR_CONSTANT(ctx->scope, 0);
			}else{
				t = stmt_new_wrapper(case, ctx->scope);
				t->expr = a;
				t->parent = ctx->switch_target;
			}

			EAT(token_colon);
			t = stmt_set_where(parse_label_next(t, ctx), &cse_loc);
			break;
		}

		default:
			if(curtok == token_identifier && tok_at_label()){
				t = parse_label(ctx);
			}else{
				t = expr_to_stmt(parse_expr_exp(ctx->scope, 0), ctx->scope);
				fold_stmt(t);
				EAT(token_semicolon);
			}
			break;
	}

	apply_stmt_attrs(t, attrs);

	return t;
}

stmt *parse_stmt(const struct stmt_ctx *ctx)
{
	attribute **attrs = NULL;
	return parse_stmt_with_attrs(ctx, &attrs);
}

static stmt *parse_stmt_and_decls(
		const struct stmt_ctx *const ctx,
		int nested_scope,
		attribute ***const attr)
{
	stmt *const code_stmt = stmt_new_wrapper(
			code, symtab_new(ctx->scope, where_cc1_current(NULL)));
	struct stmt_ctx subctx = *ctx;
	int got_decls = 0, got_attribute;

	code_stmt->symtab->internal_nest = nested_scope;

	subctx.scope = code_stmt->symtab;

	parse_local_labels(&subctx);

	parse_static_assert(subctx.scope);

	/* look for statement attributes first */
	parse_add_attr_out(attr, ctx->scope, &got_attribute);

	/* statement, or decl? */
	if(parse_at_decl(ctx->scope, /*include_attribute:*/ 0)
	|| (got_attribute && curtok != token_semicolon)
	){
		while(1){
			decl **decls = NULL;
			int new_group = parse_decl_group(
					DECL_MULTI_ACCEPT_FUNC_DECL
					| DECL_MULTI_ALLOW_STORE
					| DECL_MULTI_ALLOW_ALIGNAS
					| (got_attribute ? DECL_MULTI_CAN_DEFAULT : 0),
					/*newdecl_context:*/1,
					subctx.scope,
					subctx.scope,
					*attr ? &decls : NULL);

			if(decls){
				decl **i;
				for(i = decls; *i; i++){
					decl *d = *i;

					attribute_array_retain(*attr);
					dynarray_add_array(&d->attr, *attr);
				}
			}

			if(new_group){
				got_decls = 1;
			}else{
				break;
			}

			/* passed attrs onto decls, reset */
			got_attribute = 0;
			attribute_array_release(attr);
			*attr = NULL;
		}
	}

	if(got_decls)
		fold_shadow_dup_check_block_decls(subctx.scope);

	if(curtok != token_close_block){
		/* fine with a normal statement */
		int at_decl = 0;

		for(;;){
			stmt *this;
			where *static_asserts;

			parse_add_attr_out(attr, ctx->scope, &got_attribute);

			static_asserts = parse_static_assert(subctx.scope);
			if(static_asserts && got_attribute){
				warn_at_print_error(static_asserts, "fallthrough attribute on static-assert");
				fold_had_error = 1;

				attribute_array_release(attr);
				*attr = NULL;
			}

			parse_add_attr_out(attr, ctx->scope, &got_attribute);

			/* check for a following colon, in the case of
			 * typedef int x;
			 * x:;
			 *
			 * we check this here, as in some contexts we we always want a type,
			 * e.g. _Generic(expr, typedef_name: ...)
			 *                     ^~~~~~~~~~~~~
			 * labels are checked for in two places:
			 * 1) here, to disambiguate from decls
			 * 2) in parse_stmt() where we look for a standalone label and aren't
			 *    bothered about decls
			 */
			if(curtok == token_identifier && tok_at_label()){
				this = parse_label(&subctx);

				if(got_attribute){
					warn_at_print_error(&this->where, "fallthrough attribute on %s", this->f_str());
					fold_had_error = 1;

					attribute_array_release(attr);
					*attr = NULL;
				}
			}else if(curtok == token_close_block){
				assert(!got_attribute);
				break;
			}else{
				if((at_decl = parse_at_decl(subctx.scope, /*include_attribute:*/0))){
					assert(!got_attribute);
					break;
				}else{
					this = parse_stmt_with_attrs(&subctx, attr);
					assert(!*attr);
				}
			}

			dynarray_add(&code_stmt->bits.code.stmts, this);
		}

		if(at_decl){
			if(code_stmt->bits.code.stmts){
				stmt *nest = parse_stmt_and_decls(&subctx, 1, attr);

				if(cc1_std < STD_C99){
					static int warned = 0;
					if(!warned){
						warned = 1;
						cc1_warn_at(&nest->where, mixed_code_decls,
								"mixed code and declarations");
					}
				}

				dynarray_add(&code_stmt->bits.code.stmts, nest);
			}else{
				ICE("got another decl - should've been handled already");
			}
		}
	}

	where_cc1_current(&code_stmt->where_cbrace);

	attribute_array_release(attr);
	*attr = NULL;

	return code_stmt;
}

#ifdef SYMTAB_DEBUG
static int print_indent = 0;

#define INDENT(...) indent(), fprintf(stderr, __VA_ARGS__)

static void indent(const struct stmt_ctx *const ctx)
{
	for(int c = print_indent; c; c--)
		fputc('\t', stderr);
}

static void print_stmt_and_decls(stmt *t)
{
	INDENT("decls: (symtab %p, parent %p)\n", t->symtab, t->symtab->parent);

	print_indent++;
	for(decl **i = t->symtab->decls; i && *i; i++)
		INDENT("%s\n", (*i)->spel);
	print_indent--;

	INDENT("codes:\n");
	for(stmt **i = t->bits.code.stmts; i && *i; i++){
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
}
#endif

stmt *parse_stmt_block(symtable *scope, const struct stmt_ctx *const ctx)
{
	stmt *t;
	struct stmt_ctx subctx;
	attribute **attrs = NULL;

	if(ctx){
		subctx = *ctx;
	}else{
		memset(&subctx, 0, sizeof subctx);
		subctx.scope = scope;
	}

	EAT(token_open_block);

	t = parse_stmt_and_decls(&subctx, 0, &attrs);

	EAT(token_close_block);

#ifdef SYMTAB_DEBUG
	fprintf(stderr, "Parsed statement block:\n");
	print_stmt_and_decls(t);
#endif

	return t;
}

symtable_gasm *parse_gasm(void)
{
	symtable_gasm *g;
	struct cstring *cstr;
	where w;

	where_cc1_current(&w);

	EAT(token_open_paren);

	cstr = parse_asciz_str();
	if(!cstr)
		return NULL;

	g = umalloc(sizeof *g);
	g->asm_str = cstring_detach(cstr);
	memcpy_safe(&g->where, &w);

	EAT(token_string);
	EAT(token_close_paren);
	EAT(token_semicolon);

	return g;
}
