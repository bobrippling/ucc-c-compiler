#include <stdio.h>
#include <stdarg.h>

#include "tree.h"
#include "macros.h"


#define PRINT_IF(x, sub, fn) \
	if(x->sub){ \
		idt_printf(#sub ":\n"); \
		indent++; \
		fn(x->sub); \
		indent--; \
	}


static int indent = 0;

void idt_printf(const char *fmt, ...)
{
	va_list l;
	int i;

	for(i = indent; i > 0; i--)
		fputs("  ", stdout);

	va_start(l, fmt);
	vprintf(fmt, l);
	va_end(l);
}

void printdecl(decl *d)
{
	idt_printf("{ spel=\"%s\", ptr_depth=%d, type=%s }\n",
			d->spel, d->ptr_depth,
			d->type == type_int ? "int" :
			d->type == type_byte ? "byte" :
			d->type == type_byte ? "void" :
			"unknown"
			);
}

const char *expr_to_str(enum expr_type t)
{
	switch(t){
		CASE_STR(expr_op);
		CASE_STR(expr_val);
		CASE_STR(expr_addr);
		CASE_STR(expr_sizeof);
		CASE_STR(expr_str);
		CASE_STR(expr_identifier);
		CASE_STR(expr_assign);
		CASE_STR(expr_funcall);
	}
	return NULL;
}

const char *op_to_str(enum op_type o)
{
	switch(o){
		CASE_STR(op_multiply);
		CASE_STR(op_divide);
		CASE_STR(op_plus);
		CASE_STR(op_minus);
		CASE_STR(op_modulus);
		CASE_STR(op_deref);
		CASE_STR(op_eq);
		CASE_STR(op_ne);
		CASE_STR(op_le);
		CASE_STR(op_lt);
		CASE_STR(op_ge);
		CASE_STR(op_gt);
		CASE_STR(op_or);
		CASE_STR(op_and);
		CASE_STR(op_orsc);
		CASE_STR(op_andsc);
		CASE_STR(op_not);
		CASE_STR(op_bnot);
		CASE_STR(op_unknown);
	}
	return NULL;
}

void printexpr(expr *e)
{
	idt_printf("e->type: %s\n", expr_to_str(e->type));

	switch(e->type){
		case expr_identifier:
			idt_printf("identifier: \"%s\"\n", e->spel);
			break;

		case expr_val:
			idt_printf("val: %d\n", e->val);
			break;

		case expr_op:
			idt_printf("op: %s\n", op_to_str(e->op));
			PRINT_IF(e, lhs, printexpr);
			PRINT_IF(e, rhs, printexpr);
			break;

		case expr_str:
			idt_printf("str: \"%s\"\n", e->spel);
			break;

		case expr_assign:
			idt_printf("assign to %s:\n", e->spel);
			indent++;
			printexpr(e->expr);
			indent--;
			break;

		case expr_funcall:
		{
			expr **iter;

			idt_printf("%s():\n", e->spel);
			indent++;
			if(e->funcargs)
				for(iter = e->funcargs; *iter; iter++){
					idt_printf("arg:\n");
					indent++;
					printexpr(*iter);
					indent--;
				}
			else
				idt_printf("no args\n");
			indent--;
			break;
		}

		default:
			break;
	}
}

const char *stat_to_str(enum stat_type t)
{
	switch(t){
		CASE_STR(stat_do);
		CASE_STR(stat_if);
		CASE_STR(stat_else);
		CASE_STR(stat_while);
		CASE_STR(stat_for);
		CASE_STR(stat_break);
		CASE_STR(stat_return);
		CASE_STR(stat_expr);
		CASE_STR(stat_noop);
		CASE_STR(stat_code);
	}
	return NULL;
}

void printtree(tree *t)
{
	idt_printf("t->type: %s\n", stat_to_str(t->type));

	PRINT_IF(t, lhs,  printtree);
	PRINT_IF(t, rhs,  printtree);
	PRINT_IF(t, expr, printexpr);

	if(t->decls){
		decl **iter;

		idt_printf("decls:\n");
		for(iter = t->decls; *iter; iter++){
			indent++;
			printdecl(*iter);
			indent--;
		}
	}

	if(t->codes){
		tree **iter;

		idt_printf("code(s):\n");
		for(iter = t->codes; *iter; iter++){
			indent++;
			printtree(*iter);
			indent--;
		}
	}
}

void printfn(function *f)
{
	idt_printf("function: decl: ");
	printdecl(f->func_decl);

	PRINT_IF(f, code, printtree)
}
