#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>

#include "../util/util.h"
#include "data_structs.h"
#include "macros.h"
#include "sym.h"
#include "cc1.h"
#include "sue.h"
#include "gen_str.h"

#define ENGLISH_PRINT_ARGLIST

#define PRINT_IF(x, sub, fn) \
	if(x->sub){ \
		idt_printf(#sub ":\n"); \
		gen_str_indent++; \
		fn(x->sub); \
		gen_str_indent--; \
	}

int gen_str_indent = 0;

void idt_print()
{
	int i;

	for(i = gen_str_indent; i > 0; i--)
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
	UCC_ASSERT(expr_kind(e, val), "%s: not a value expression", __func__);
	UCC_ASSERT((e->val.iv.suffix & VAL_UNSIGNED) == 0, "TODO: unsigned");
	fprintf(cc1_out, "%ld", e->val.iv.val);
}

#if 0
void print_decl_ptr_eng(decl_ptr *dp)
{
	if(dp->child){
		print_decl_ptr_eng(dp->child);

		fprintf(cc1_out, "%s%s",
				dp->array_size ? "" : "pointer to ",
				type_qual_to_str(dp->qual));
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
	if(decl_spel(d))
		fprintf(cc1_out, "\"%s\": ", decl_spel(d));

	print_decl_ptr_eng(d->decl_ptr);

	fprintf(cc1_out, "%s", type_to_str(d->type));
}
#endif

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

void print_decl_desc(decl_desc *dp, decl *d)
{
	switch(dp->type){
		case decl_desc_ptr:
			fprintf(cc1_out, "*%s", type_qual_to_str(dp->bits.qual));
			break;

		case decl_desc_array:
			/* done below */
			break;

		case decl_desc_func:
			if(dp->child)
				fputc('(', cc1_out);
			break;
	}

	if(dp->child)
		print_decl_desc(dp->child, d);
	else if(d->spel)
		fputs(d->spel, cc1_out);

	switch(dp->type){
		case decl_desc_func:
			if(dp->child)
				fputc(')', cc1_out);

			print_funcargs(dp->bits.func);
			break;

		case decl_desc_array:
			fputc('[', cc1_out);
			print_expr_val(dp->bits.array_size);
			fputc(']', cc1_out);
			break;

		case decl_desc_ptr:
			break;
	}
}

void print_decl(decl *d, enum pdeclargs mode)
{
	if(mode & PDECL_INDENT)
		idt_print();

	if((mode & PDECL_PIGNORE) && d->ignore)
		fprintf(cc1_out, "(ignored) ");

	if(fopt_mode & FOPT_ENGLISH){
		//print_decl_eng(d);
	}else{
		fputs(type_to_str(d->type), cc1_out);

		if(d->type->typeof){
			fputc('\n', cc1_out);
			gen_str_indent++;
			idt_printf("typeof expr:\n");
			gen_str_indent++;
			print_expr(d->type->typeof);
			gen_str_indent -= 2;
			idt_print();
		}

#if 0
		if(fopt_mode & FOPT_DECL_PTR_STAT){
			const int idt_orig = gen_str_indent;
			decl_desc *dpi;

			fputc('\n', cc1_out);
			for(dpi = d->decl_ptr; dpi; dpi = dpi->child){
				gen_str_indent++;
				idt_printf("decl_ptr: %s%s",
						type_qual_to_str(dpi->qual),
						dpi->fptrargs ? "(#)" : "");

				if(dpi->array_size){
					fputs("\n", cc1_out);
					idt_printf("array size:\n");
					gen_str_indent++;
					print_expr(dpi->array_size);
					gen_str_indent--;
				}
			}

			gen_str_indent = idt_orig;
		}else{
#endif
			if(d->desc){
				fputc(' ', cc1_out);
				print_decl_desc(d->desc, d);
			}
#if 0
		}
#endif
	}

	if(mode & PDECL_SYM_OFFSET){
		if(d->sym)
			fprintf(cc1_out, " (sym offset = %d)", d->sym->offset);
		else
			fprintf(cc1_out, " (no sym)");
	}

	if(mode & PDECL_NEWLINE)
		fputc('\n', cc1_out);

	if(mode & PDECL_PINIT){
		gen_str_indent++;
		print_expr(d->init);
		gen_str_indent--;
	}

	if((mode & PDECL_FUNC_DESCEND) && d->func_code){
		decl **iter;

		gen_str_indent++;

		for(iter = d->func_code->symtab->decls; iter && *iter; iter++)
			idt_printf("offset of %s = %d\n", decl_spel(*iter), (*iter)->sym->offset);

		idt_printf("function stack space %d\n", d->func_code->symtab->auto_total_size);

		print_stmt(d->func_code);

		gen_str_indent--;
	}
}

void print_sym(sym *s)
{
	idt_printf("sym: type=%s, offset=%d, type: ", sym_to_str(s->type), s->offset);
	print_decl(s->decl, PDECL_NEWLINE | PDECL_PIGNORE);
}

void print_expr(expr *e)
{
	idt_printf("expr: %s\n", e->f_str());
	if(e->tree_type){ /* might be a label */
		idt_printf("tree_type: ");
		gen_str_indent++;
		print_decl(e->tree_type, PDECL_NEWLINE);
		gen_str_indent--;
	}
	gen_str_indent++;
	e->f_gen(e, NULL);
	gen_str_indent--;
}

void print_struct(struct_union_enum_st *sue)
{
	sue_member **iter;

	idt_printf("%s %s (size %d):\n", sue_str(sue), sue->spel, struct_union_size(sue));

	gen_str_indent++;
	for(iter = sue->members; iter && *iter; iter++){
		decl *d = &(*iter)->struct_member;
		idt_printf("offset %d:\n", d->struct_offset);
		gen_str_indent++;
		print_decl(d, PDECL_INDENT | PDECL_NEWLINE);
		gen_str_indent--;
	}
	gen_str_indent--;
}

void print_enum(struct_union_enum_st *et)
{
	sue_member **mi;

	idt_printf("enum %s:\n", et->spel);

	gen_str_indent++;
	for(mi = et->members; *mi; mi++){
		enum_member *m = &(*mi)->enum_member;

		idt_printf("member %s = %d\n", m->spel, m->val->val.iv.val);
	}
	gen_str_indent--;
}

int has_st_en_tdef(symtable *stab)
{
	return stab->sues || stab->typedefs;
}

void print_st_en_tdef(symtable *stab)
{
	struct_union_enum_st **sit;

	for(sit = stab->sues; sit && *sit; sit++){
		struct_union_enum_st *sue = *sit;
		(sue->primitive == type_enum ? print_enum : print_struct)(sue);
	}

	if(stab->typedefs){
		decl **tit;

		idt_printf("typedefs:\n");
		gen_str_indent++;
		for(tit = stab->typedefs; tit && *tit; tit++)
			print_decl(*tit, PDECL_INDENT | PDECL_NEWLINE);
		gen_str_indent--;
	}
}

void print_stmt_flow(stmt_flow *t)
{
	idt_printf("flow:\n");
	if(t->for_init_decls){
		idt_printf("inits:\n");
		decl **i;
		gen_str_indent++;

		for(i = t->for_init_decls; *i; i++)
			print_decl(*i, PDECL_INDENT
					| PDECL_NEWLINE
					| PDECL_SYM_OFFSET
					| PDECL_PIGNORE
					| PDECL_PINIT);

		gen_str_indent--;
	}

	idt_printf("for parts:\n");
	gen_str_indent++;
	PRINT_IF(t, for_init,      print_expr);
	PRINT_IF(t, for_while,     print_expr);
	PRINT_IF(t, for_inc,       print_expr);
	gen_str_indent--;
}

void print_stmt(stmt *t)
{
	idt_printf("statement: %s\n", t->f_str());

	if(t->flow){
		gen_str_indent++;
		print_stmt_flow(t->flow);
		gen_str_indent--;
	}

	PRINT_IF(t, expr, print_expr);
	PRINT_IF(t, lhs,  print_stmt);
	PRINT_IF(t, rhs,  print_stmt);
	PRINT_IF(t, rhs,  print_stmt);

	if(t->symtab && has_st_en_tdef(t->symtab)){
		idt_printf("structs, enums and tdefs in this block:\n");
		gen_str_indent++;
		print_st_en_tdef(t->symtab);
		gen_str_indent--;
	}

	if(t->decls){
		decl **iter;

		idt_printf("stack space %d\n", t->symtab->auto_total_size);
		idt_printf("decls:\n");

		for(iter = t->decls; *iter; iter++){
			decl *d = *iter;

			gen_str_indent++;
			print_decl(d, PDECL_INDENT | PDECL_NEWLINE | PDECL_SYM_OFFSET | PDECL_PIGNORE);
			gen_str_indent--;
		}
	}

	if(t->codes){
		stmt **iter;

		idt_printf("code(s):\n");
		for(iter = t->codes; *iter; iter++){
			gen_str_indent++;
			print_stmt(*iter);
			gen_str_indent--;
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
			gen_str_indent++;
			print_expr((*diter)->init);
			gen_str_indent--;
		}

		fputc('\n', cc1_out);
	}
}
