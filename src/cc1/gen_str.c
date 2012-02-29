#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>

#include "../util/util.h"
#include "data_structs.h"
#include "macros.h"
#include "sym.h"
#include "cc1.h"
#include "struct.h"
#include "enum.h"

#define ENGLISH_PRINT_ARGLIST

#define PRINT_IF(x, sub, fn) \
	if(x->sub){ \
		idt_printf(#sub ":\n"); \
		indent++; \
		fn(x->sub); \
		indent--; \
	}

static int indent = 0;

enum pdeclargs
{
	PDECL_NONE          = 0,
	PDECL_INDENT        = 1 << 0,
	PDECL_NEWLINE       = 1 << 1,
	PDECL_SYM_OFFSET    = 1 << 2,
	PDECL_FUNC_DESCEND  = 1 << 3,
	PDECL_PIGNORE       = 1 << 4,
};

void print_decl(decl *d, enum pdeclargs);

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

void print_expr_val(expr *e)
{
	UCC_ASSERT(e->type == expr_val, "%s: not a value expression", __func__);
	fprintf(cc1_out, "%ld", e->val.i.val);
}

void print_decl_ptr_eng(decl_ptr *dp)
{
	if(dp->child){
		print_decl_ptr_eng(dp->child);

		fprintf(cc1_out, "%s%s",
				dp->array_size ? "" : "pointer to ",
				dp->is_const    ? "const " : "");
	}

	if(dp->fptrargs){
#ifdef ENGLISH_PRINT_ARGLIST
		funcargs *fargs = dp->fptrargs;
		decl **iter;
#endif

		fputs("function", cc1_out);

#ifdef ENGLISH_PRINT_ARGLIST
		fputc('(', cc1_out);
		if(fargs->arglist){

			for(iter = fargs->arglist; iter && *iter; iter++){
				print_decl(*iter, PDECL_NONE);
				if(iter[1])
					fputs(", ", cc1_out);
			}

			if(fargs->variadic)
				fputs("variadic", cc1_out);

		}else{
			fprintf(cc1_out, "taking %s arguments", fargs->args_void ? "no" : "unspecified");
		}
		fputc(')', cc1_out);
#endif
		fputs(" returning ", cc1_out);
	}
	if(dp->array_size){
		fputs("array[", cc1_out);
		print_expr_val(dp->array_size);
		fputs("] of ", cc1_out);
	}
}

void print_decl_eng(decl *d)
{
	if(d->spel)
		fprintf(cc1_out, "\"%s\": ", d->spel);

	print_decl_ptr_eng(d->decl_ptr);

	fprintf(cc1_out, "%s", type_to_str(d->type));
}

void print_funcargs(funcargs *fargs)
{
	fputc('(', cc1_out);
	if(fargs->arglist){
		decl **iter;

		for(iter = fargs->arglist; *iter; iter++){
			print_decl(*iter, PDECL_NONE);
			if(iter[1])
				fputs(", ", cc1_out);
		}
	}else if(fargs->args_void){
		fputs("void", cc1_out);
	}

	fprintf(cc1_out, "%s)", fargs->variadic ? ", ..." : "");
}

void print_decl_ptr(decl_ptr *dp, decl *parent)
{
	fprintf(cc1_out, "*%s%s", dp->is_const ? "const " : "", dp->array_size ? "[]" : "");

	if(dp->child){
		if(dp->fptrargs)
			fputc('(', cc1_out);

		print_decl_ptr(dp->child, parent);
	}else if(parent->spel){
		fputs(parent->spel, cc1_out);
	}

	if(dp->fptrargs){
		if(dp->child)
			fputc(')', cc1_out);

		print_funcargs(dp->fptrargs);
	}
	if(dp->array_size){
		fputc('[', cc1_out);
		print_expr_val(dp->array_size);
		fputc(']', cc1_out);
	}
}

void print_decl(decl *d, enum pdeclargs mode)
{
	if(mode & PDECL_INDENT)
		idt_print();

	if((mode & PDECL_PIGNORE) && d->ignore)
		fprintf(cc1_out, "(ignored) ");

	if(fopt_mode & FOPT_ENGLISH){
		print_decl_eng(d);
	}else{
		fputs(type_to_str(d->type), cc1_out);

		if(fopt_mode & FOPT_DECL_PTR_TREE){
			const int idt_orig = indent;
			decl_ptr *dpi;

			fputc('\n', cc1_out);
			for(dpi = d->decl_ptr; dpi; dpi = dpi->child){
				indent++;
				idt_printf("decl_ptr: %s%s",
						dpi->is_const ? "const" : "",
						dpi->fptrargs ? "(#)" : "");

				if(dpi->array_size){
					fputs("\n", cc1_out);
					idt_printf("array size:\n");
					indent++;
					print_expr(dpi->array_size);
					indent--;
				}
			}

			indent = idt_orig;
		}else{
			if(d->spel)
				fputc(' ', cc1_out);

			if(d->decl_ptr)
				print_decl_ptr(d->decl_ptr, d); /* handles spel */
			else if(d->spel)
				fputs(d->spel, cc1_out);
		}

		if(d->funcargs)
			print_funcargs(d->funcargs);
	}

	if(mode & PDECL_SYM_OFFSET){
		if(d->sym)
			fprintf(cc1_out, " (sym offset = %d)", d->sym->offset);
		else
			fprintf(cc1_out, " (no sym)");
	}

	if(mode & PDECL_NEWLINE)
		fputc('\n', cc1_out);

	if((mode & PDECL_FUNC_DESCEND) && d->func_code){
		decl **iter;

		indent++;

		for(iter = d->func_code->symtab->decls; iter && *iter; iter++)
			idt_printf("offset of %s = %d\n", (*iter)->spel, (*iter)->sym->offset);

		idt_printf("function stack space %d\n", d->func_code->symtab->auto_total_size);

		print_tree(d->func_code);

		indent--;
	}
}

void print_sym(sym *s)
{
	idt_printf("sym: type=%s, offset=%d, type: ", sym_to_str(s->type), s->offset);
	print_decl(s->decl, PDECL_NEWLINE | PDECL_PIGNORE);
}

void print_expr(expr *e)
{
	idt_printf("e->type: %s\n", expr_to_str(e->type));
	idt_printf("tree_type: ");
	indent++;
	print_decl(e->tree_type, PDECL_NEWLINE);
	indent--;

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
			idt_printf("identifier: \"%s\" (sym %p)\n", e->spel, e->sym);
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

			idt_printf("funcall, calling:\n");

			indent++;
			print_expr(e->expr);
			indent--;

			if(e->funcargs){
				int i;
				idt_printf("args:\n");
				indent++;
				for(i = 1, iter = e->funcargs; *iter; iter++, i++){
					idt_printf("arg %d:\n", i);
					indent++;
					print_expr(*iter);
					indent--;
				}
				indent--;
			}else{
				idt_printf("no args\n");
			}
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
				idt_printf("address of expr:\n");
				indent++;
				print_expr(e->expr);
				indent--;
			}
			break;

			indent--;
			break;

		case expr_sizeof:
			if(e->expr->expr_is_sizeof){
				idt_printf("sizeof %s\n", decl_to_str(e->expr->tree_type));
			}else{
				idt_printf("sizeof expr:\n");
				print_expr(e->expr);
			}
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

void print_struct(struct_st *st)
{
	decl **iter;

	idt_printf("struct %s (size %d):\n", st->spel, struct_size(st));

	indent++;
	for(iter = st->members; iter && *iter; iter++){
		decl *d = *iter;
		print_decl(d, PDECL_INDENT | PDECL_NEWLINE);
		idt_printf("offset %d\n", d->struct_offset);
	}
	indent--;
}

void print_enum(enum_st *et)
{
	enum_member **mi;

	idt_printf("enum %s:\n", et->spel);

	indent++;
	for(mi = et->members; *mi; mi++){
		enum_member *m = *mi;
		idt_printf("member %s = %d\n", m->spel, m->val->val.i.val);
	}
	indent--;
}

void print_st_en_tdef(symtable *stab)
{
	struct_st **sit;
	enum_st   **eit;
	decl      **tit;

	/* fold structs, then enums, then decls - decls may rely on enums */
	for(sit = stab->structs; sit && *sit; sit++)
		print_struct(*sit);

	for(eit = stab->enums; eit && *eit; eit++)
		print_enum(*eit);

	if(stab->typedefs){
		idt_printf("typedefs:\n");
		indent++;
		for(tit = stab->typedefs; tit && *tit; tit++)
			print_decl(*tit, PDECL_INDENT | PDECL_NEWLINE);
		indent--;
	}
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

		idt_printf("stack space %d\n", t->symtab->auto_total_size);
		idt_printf("decls:\n");

		for(iter = t->decls; *iter; iter++){
			decl *d = *iter;

			indent++;
			print_decl(d, PDECL_INDENT | PDECL_NEWLINE | PDECL_SYM_OFFSET | PDECL_PIGNORE);
			indent--;
		}
	}

	print_st_en_tdef(t->symtab);

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

void gen_str(symtable *symtab)
{
	decl **diter;

	print_st_en_tdef(symtab);

	for(diter = symtab->decls; diter && *diter; diter++){
		print_decl(*diter, PDECL_INDENT | PDECL_NEWLINE | PDECL_PIGNORE | PDECL_FUNC_DESCEND);
		if((*diter)->init){
			idt_printf("init:\n");
			indent++;
			print_expr((*diter)->init);
			indent--;
		}

		fputc('\n', cc1_out);
	}
}
