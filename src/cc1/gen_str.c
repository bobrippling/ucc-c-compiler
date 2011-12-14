#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>

#include "../util/util.h"
#include "tree.h"
#include "macros.h"
#include "sym.h"
#include "cc1.h"


#define PRINT_IF(x, sub, fn) \
	if(x->sub){ \
		idt_printf(#sub ":\n"); \
		indent++; \
		fn(x->sub); \
		indent--; \
	}

static int indent = 0;

void print_tree(tree *t);
void print_expr(expr *e);
void print_func(decl *d);
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

void print_decl(decl *d, int idt, int nl, int sym_offset)
{
	int i;

	if(idt)
		idt_print();

	if(d->ignore)
		fprintf(cc1_out, "((extern) ignored/overridden later) ");

	fputs(type_to_str(d->type), cc1_out);

	if(d->ptr_depth || d->spel)
		fputc(' ', cc1_out);

	for(i = d->ptr_depth; i > 0; i--)
		fputc('*', cc1_out);

	if(d->spel)
		fputs(d->spel, cc1_out);

	if(sym_offset && d->sym)
		fprintf(cc1_out, " (sym offset = %d)", d->sym->offset);

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
	print_decl(s->decl, 0, 1, 0);
}

void print_expr(expr *e)
{
	idt_printf("vartype: ");
	print_decl(e->tree_type, 0, 1, 0);

	idt_printf("e->type: %s\n", expr_to_str(e->type));

	switch(e->type){
		case expr_comma:
			idt_printf("comma expression\n");
			idt_printf("comma lhs:\n");
			indent++;
			print_expr(e->lhs);
			indent--;
			idt_printf("comma rhs:\n");
			indent++;
			print_expr(e->rhs);
			indent--;
			break;

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
				idt_printf("deref size: %s ", decl_to_str(e->tree_type));
				fputc('\n', cc1_out);
			}

			PRINT_IF(e, lhs, print_expr);
			PRINT_IF(e, rhs, print_expr);
			indent--;
			break;

		case expr_assign:
			idt_printf("%sassignment, expr:\n", e->assign_is_post ? "post-inc/dec " : "");
			idt_printf("assign to:\n");
			indent++;
			print_expr(e->lhs);
			indent--;
			idt_printf("assign from:\n");
			indent++;
			print_expr(e->rhs);
			indent--;
			break;

		case expr_funcall:
		{
			expr **iter;

			idt_printf("%s():\n", e->spel);
			indent++;
			if(e->funcargs){
				int i;
				for(i = 1, iter = e->funcargs; *iter; iter++, i++){
					idt_printf("arg %d:\n", i);
					indent++;
					print_expr(*iter);
					indent--;
				}
			}else{
				idt_printf("no args\n");
			}
			indent--;
			break;
		}

		case expr_addr:
			if(e->array_store){
				if(e->array_store->type == array_str){
					idt_printf("label: %s, \"%s\" (length=%d)\n", e->array_store->label, e->array_store->data.str, e->array_store->len);
				}else{
					int i;
					idt_printf("array: %s:\n", e->array_store->label);
					indent++;
					for(i = 0; e->array_store->data.exprs[i]; i++){
						idt_printf("array[%d]:\n", i);
						indent++;
						print_expr(e->array_store->data.exprs[i]);
						indent--;
					}
					indent--;
				}
			}else{
				idt_printf("&%s\n", e->spel);
			}
			break;

			indent--;
			break;

		case expr_sizeof:
			idt_printf("sizeof %s\n", decl_to_str(e->tree_type));
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
			if((*iter)->func)
				print_func(*iter);
			else
				print_decl(*iter, 1, 1, 1);
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

void print_func(decl *d)
{
	function *f = d->func;
	decl **iter;

	idt_printf("function: ");
	indent++;

	print_decl(d, 0, 0, 0);

	fputc('(', cc1_out);
	for(iter = f->args; iter && *iter; iter++){
		print_decl(*iter, 0, 0, 0);
		fprintf(cc1_out, "%s", iter[1] ? ", " : "");
	}

	fprintf(cc1_out, "%s)\n", f->variadic ? ", ..." : "");

	if(f->code){
#define STAB f->code->symtab
		idt_printf("stack space = %d (start %d, finish %d)\n",
				STAB->auto_offset_finish - STAB->auto_offset_start,
				STAB->auto_offset_start,
				STAB->auto_offset_finish
				);

		for(iter = f->args; iter && *iter; iter++)
			idt_printf(" offset of %s = %d\n", (*iter)->spel, (*iter)->sym->offset);

		PRINT_IF(f, code, print_tree)
	}

	indent--;
}

void gen_str(symtable *symtab)
{
	decl **diter;
	for(diter = symtab->decls; diter && *diter; diter++){
		if((*diter)->func){
			print_func(*diter);
		}else{
			fprintf(cc1_out, "global variable:\n");

			indent++;
			print_decl(*diter, 0, 1, 0);
			if((*diter)->init){
				indent++;
				fprintf(cc1_out, "init:\n");
				indent++;
				print_expr((*diter)->init);
				indent -= 2;
			}
			indent--;
		}
		fputc('\n', cc1_out);
	}
}
