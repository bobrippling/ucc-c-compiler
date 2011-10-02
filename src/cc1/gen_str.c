#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>

#include "tree.h"
#include "macros.h"
#include "sym.h"


#define PRINT_IF(x, sub, fn) \
	if(x->sub){ \
		idt_printf(#sub ":\n"); \
		indent++; \
		fn(x->sub); \
		indent--; \
	}

static int indent = 0;
extern FILE *cc1_out;

void print_tree(tree *t);
void print_expr(expr *e);
void idt_print(void);

void idt_print()
{
	int i;

	for(i = indent; i > 0; i--)
		fputs("  ", cc1_out);
}

void idt_printf(const char *fmt, ...)
{
	va_list l;

	idt_print();

	va_start(l, fmt);
	vfprintf(cc1_out, fmt, l);
	va_end(l);
}

void print_type(type *t)
{
	int i;

	if(t->spec & spec_const)
		fputs("const ", cc1_out);
	if(t->spec & spec_extern)
		fputs("extern ", cc1_out);

	fprintf(cc1_out, "%s ", type_to_str(t));

	for(i = t->ptr_depth; i > 0; i--)
		fputc('*', cc1_out);
}

void print_decl(decl *d, int idt, int nl)
{
	int i;

	if(idt)
		idt_print();

	print_type(d->type);

	if(d->spel)
		fputs(d->spel, cc1_out);

	if(nl)
		fputc('\n', cc1_out);

	indent++;
	for(i = 0; d->arraysizes && d->arraysizes[i]; i++){
		idt_printf("array[%d] size:\n", i);
		indent++;
		print_expr(d->arraysizes[i]);
		indent--;
	}
	indent--;
}

void print_sym(sym *s)
{
	idt_printf("sym: type=%s, offset=%d, type: ", sym_to_str(s->type), s->offset);
	print_decl(s->decl, 0, 1);
}

void print_expr(expr *e)
{
	idt_printf("vartype: ");
	print_type(e->vartype);
	fputc('\n', cc1_out);

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
			indent++;

			if(e->op == op_deref){
				int i;
				idt_printf("deref size: %s ", type_to_str(e->vartype));
				for(i = 0; i < e->vartype->ptr_depth; i++)
					fputc('*', cc1_out);
				fputc('\n', cc1_out);
			}

			PRINT_IF(e, lhs, print_expr);
			PRINT_IF(e, rhs, print_expr);
			indent--;
			break;

		case expr_str:
			idt_printf("str: %s, \"%s\" (length=%d)\n", e->sym->str_lbl, e->val.s, e->strl);
			break;

		case expr_assign:
			idt_printf("%s assignment, expr:\n", assign_to_str(e->assign_type));
			if(e->assign_type == assign_normal){
				idt_printf("assign to:\n");
				indent++;
				print_expr(e->lhs);
				indent--;
				idt_printf("assign from:\n");
				indent++;
				print_expr(e->rhs);
				indent--;
			}else{
				indent++;
				print_expr(e->expr);
				indent--;
			}
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
					print_expr(*iter);
					indent--;
				}
			else
				idt_printf("no args\n");
			indent--;
			break;
		}

		case expr_addr:
			indent++;
			idt_printf("&%s\n", e->spel);
			indent--;
			break;

		case expr_sizeof:
			idt_printf("sizeof %s\n", e->spel ? e->spel : type_to_str(e->vartype));
			break;

		case expr_cast:
			idt_printf("cast expr:\n");
			indent++;
			print_expr(e->rhs);
			indent--;
			break;

		case expr_if:
			idt_printf("if expression:\n");
			indent++;
#define SUB_PRINT(nam) \
			do{\
				idt_printf(#nam  ":\n"); \
				indent++; \
				print_expr(e->nam); \
				indent--; \
			}while(0)

			SUB_PRINT(expr);
			if(e->lhs)
				SUB_PRINT(lhs);
			else
				idt_printf("?: syntactic sugar\n");

			SUB_PRINT(rhs);
#undef SUB_PRINT
			break;

		default:
			idt_printf("\x1b[1;31m%s not handled!\x1b[m\n", expr_to_str(e->type));
			break;
	}
}

void print_tree_flow(tree_flow *t)
{
	idt_printf("flow:\n");
	indent++;
	PRINT_IF(t, for_init,  print_expr);
	PRINT_IF(t, for_while, print_expr);
	PRINT_IF(t, for_inc,   print_expr);
	indent--;
}

void print_tree(tree *t)
{
	idt_printf("t->type: %s\n", stat_to_str(t->type));

	if(t->flow){
		indent++;
		print_tree_flow(t->flow);
		indent--;
	}

	PRINT_IF(t, expr, print_expr);
	PRINT_IF(t, lhs,  print_tree);
	PRINT_IF(t, rhs,  print_tree);

	if(t->decls){
		decl **iter;

		idt_printf("decls:\n");
		for(iter = t->decls; *iter; iter++){
			indent++;
			print_decl(*iter, 1, 1);
			indent--;
		}
	}

	if(t->codes){
		tree **iter;

		idt_printf("code(s):\n");
		for(iter = t->codes; *iter; iter++){
			indent++;
			print_tree(*iter);
			indent--;
		}
	}
}

void print_func(function *f)
{
	idt_printf("function: ");
	indent++;

	print_decl(f->func_decl, 0, 0);

	fputc('(', cc1_out);
	if(f->args){
		decl **iter;
		for(iter = f->args; *iter; iter++){
			print_decl(*iter, 0, 0);
			fprintf(cc1_out, "%s ", iter[1] ? "," : ""); /* TODO: comma and reverse order */
		}
	}

	fprintf(cc1_out, "%s)\n", f->variadic ? ", ..." : "");

	PRINT_IF(f, code, print_tree)

	indent--;
}

void gen_str(global **g)
{
	for(; *g; g++)
		if((*g)->isfunc){
			print_func((*g)->ptr.f);
		}else{
			fprintf(cc1_out, "global variable: ");
			print_decl((*g)->ptr.d, 1, 1);
		}
}
