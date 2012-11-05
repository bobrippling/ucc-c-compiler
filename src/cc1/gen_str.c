#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>

#include "../util/util.h"
#include "../util/platform.h"
#include "data_structs.h"
#include "macros.h"
#include "sym.h"
#include "cc1.h"
#include "sue.h"
#include "gen_str.h"
#include "str.h"
#include "const.h"
#include "ops/stmt_code.h" /* FOR_INIT_AND_CODE */
#include "decl_init.h"

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
	intval iv;

	const_fold_need_val(e, &iv);

	UCC_ASSERT((iv.suffix & VAL_UNSIGNED) == 0, "TODO: unsigned");

	fprintf(cc1_out, "%ld", iv.val);
}

void print_decl_init(decl_init *di)
{
	switch(di->type){
		case decl_init_scalar:
			print_expr(di->bits.expr);
			break;

		case decl_init_brace:
		{
			decl_init *s;
			int i;

			for(i = 0; (s = di->bits.inits[i]); i++){
				const int need_brace = decl_init_is_brace(s);

				/* ->member not printed */
#ifdef DINIT_WITH_STRUCT
				if(s->spel)
					idt_printf(".%s", s->spel);
				else
#endif
					idt_printf("[%d]", i);

				fprintf(cc1_out, " = %s\n", need_brace ? "{" : "");

				gen_str_indent++;
				print_decl_init(s);
				gen_str_indent--;

				if(need_brace)
					idt_printf("}\n");
			}
			break;
		}
	}
}

void print_type_ref_eng(type_ref *ref)
{
	if(!ref)
		return;

	print_type_ref_eng(ref->ref);

	switch(ref->type){
		case type_ref_cast:
			fprintf(cc1_out, "%s ", type_qual_to_str(ref->bits.qual));
			break;

		case type_ref_ptr:
			fprintf(cc1_out, "%spointer to ", type_qual_to_str(ref->bits.qual));
			break;

		case type_ref_block:
			fprintf(cc1_out, "block returning ");
			break;

		case type_ref_func:
		{
#ifdef ENGLISH_PRINT_ARGLIST
			funcargs *fargs = ref->bits.func;
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

			break;
		}

		case type_ref_array:
			if(ref->bits.array_size){
				fputs("array[", cc1_out);
				print_expr_val(ref->bits.array_size);
				fputs("] of ", cc1_out);
			}
			break;

		case type_ref_type:
			fprintf(cc1_out, "%s", type_to_str(ref->bits.type));
			break;

		case type_ref_tdef:
			ICE("TODO");
	}
}

void print_decl_eng(decl *d)
{
	if(d->spel)
		fprintf(cc1_out, "\"%s\": ", d->spel);

	print_type_ref_eng(d->ref);
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

static void print_tdef(type_ref *t)
{
	UCC_ASSERT(t->type == type_ref_tdef, "invalid tdef");

	/* TODO */
	fputc('\n', cc1_out);
	gen_str_indent++;
	idt_printf("typeof expr:\n");
	gen_str_indent++;
	print_expr(t->bits.type_of);
	gen_str_indent -= 2;
	idt_print();
}

void print_type_ref(type_ref *ref, decl *d)
{
	switch(ref->type){
		case type_ref_cast:
		case type_ref_ptr:
			fprintf(cc1_out, "%s%s", ref->type == type_ref_cast ? "" : "*", type_qual_to_str(ref->bits.qual));
			break;

		case type_ref_block:
			fputc('^', cc1_out);
			break;

		case type_ref_tdef:
			print_tdef(ref);
			break;

		case type_ref_array: /* done below */
		case type_ref_func:
		case type_ref_type:
			break;
	}

	if(ref->ref){
		const int need_paren = ref->type != ref->ref->type;

		if(need_paren)
			fputc('(', cc1_out);

		print_type_ref(ref->ref, d);

		if(need_paren)
			fputc(')', cc1_out);
	}else if(d && d->spel){
		fputs(d->spel, cc1_out);
	}

	switch(ref->type){
		case type_ref_func:
			print_funcargs(ref->bits.func);
			break;

		case type_ref_array:
		{
			intval sz;

			const_fold_need_val(ref->bits.array_size, &sz);

			if(sz.val)
				fprintf(cc1_out, "[%ld]", sz.val);
			else
				fprintf(cc1_out, "[]");
			break;
		}

		case type_ref_ptr:
		case type_ref_cast:
		case type_ref_block:
			break;
		case type_ref_tdef:
		case type_ref_type:
			ICE("TODO");
	}
}

void print_decl(decl *d, enum pdeclargs mode)
{
	if(mode & PDECL_INDENT)
		idt_print();

	if((mode & PDECL_PISDEF)){
		int one = !d->is_definition || d->inline_only;

		if(one)
			fputc('(', cc1_out);

		if(!d->is_definition)
			fprintf(cc1_out, "not definition");

		if(d->inline_only)
			fprintf(cc1_out, "%sinline-only", d->is_definition ? "" : ", ");

		if(one)
			fputc(')', cc1_out);
	}

	if(fopt_mode & FOPT_ENGLISH){
		print_decl_eng(d);
	}else{
		if(d->spel)
			fprintf(cc1_out, " %s, type ", d->spel);

		fputs(type_to_str(decl_get_type(d)), cc1_out);

		if(d->ref){
			fputc(' ', cc1_out);
			print_type_ref(d->ref, d);
		}
	}

	if(mode & PDECL_SYM_OFFSET){
		if(d->sym)
			fprintf(cc1_out, " (sym %s, offset = %d)", sym_to_str(d->sym->type), d->sym->offset);
		else
			fprintf(cc1_out, " (no sym)");
	}

	if(mode & PDECL_SIZE && !DECL_IS_FUNC(d)){
		if(decl_is_incomplete_array(d)){
			fprintf(cc1_out, " incomplete array in decl");
		}else{
			const int sz = decl_size(d);
			fprintf(cc1_out, " size %d bytes. %d platform-word(s)", sz, sz / platform_word_size());
		}
	}

	if(mode & PDECL_NEWLINE)
		fputc('\n', cc1_out);

	if(d->init && mode & PDECL_PINIT){
		gen_str_indent++;
		print_decl_init(d->init);
		gen_str_indent--;
	}

	if((mode & PDECL_ATTR) && d->attr){
		decl_attr *da = d->attr;
		gen_str_indent++;
		for(; da; da = da->next)
			idt_printf("__attribute__((%s))\n", decl_attr_to_str(da->type));
		gen_str_indent--;
	}

	if((mode & PDECL_FUNC_DESCEND) && d->func_code){
		decl **iter;

		gen_str_indent++;

		for(iter = d->func_code->symtab->decls; iter && *iter; iter++)
			idt_printf("offset of %s = %d\n", (*iter)->spel, (*iter)->sym->offset);

		idt_printf("function stack space %d\n", d->func_code->symtab->auto_total_size);

		print_stmt(d->func_code);

		gen_str_indent--;
	}
}

void print_sym(sym *s)
{
	idt_printf("sym: type=%s, offset=%d, type: ", sym_to_str(s->type), s->offset);
	print_decl(s->decl, PDECL_NEWLINE | PDECL_PISDEF);
}

void print_expr(expr *e)
{
	idt_printf("expr: %s\n", e->f_str());
	if(e->tree_type){ /* might be a label */
		idt_printf("tree_type: ");
		gen_str_indent++;
		print_type_ref(e->tree_type, NULL);
		gen_str_indent--;
	}
	gen_str_indent++;
	e->f_gen(e, NULL);
	gen_str_indent--;
}

void print_struct(struct_union_enum_st *sue)
{
	sue_member **iter;

	if(sue_incomplete(sue)){
		idt_printf("incomplete %s %s\n", sue_str(sue), sue->spel);
		return;
	}

	idt_printf("%s %s (size %d):\n", sue_str(sue), sue->spel, sue_size(sue));

	gen_str_indent++;
	for(iter = sue->members; iter && *iter; iter++){
		decl *d = (*iter)->struct_member;

		idt_printf("offset %d:\n", d->struct_offset);

#ifdef FIELD_WIDTH_TODO
		if(d->field_width){
			intval iv;

			const_fold_need_val(d->field_width, &iv);

			idt_printf("field width %ld\n", iv.val);
		}
#endif

		gen_str_indent++;
		print_decl(d, PDECL_INDENT | PDECL_NEWLINE | PDECL_ATTR);
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
		enum_member *m = (*mi)->enum_member;

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
	int nl = 0;

	for(sit = stab->sues; sit && *sit; sit++){
		struct_union_enum_st *sue = *sit;
		(sue->primitive == type_enum ? print_enum : print_struct)(sue);
		nl = 1;
	}

	if(stab->typedefs){
		decl **tit;

		idt_printf("typedefs:\n");
		gen_str_indent++;
		for(tit = stab->typedefs; tit && *tit; tit++){
			print_decl(*tit, PDECL_INDENT | PDECL_NEWLINE | PDECL_ATTR);
			nl = 1;
		}
		gen_str_indent--;
	}

	if(nl)
		fputc('\n', cc1_out);
}

void print_stmt_flow(stmt_flow *t)
{
	idt_printf("flow:\n");

	if(t->for_init_decls){
		decl **i;

		idt_printf("inits:\n");
		gen_str_indent++;

		for(i = t->for_init_decls; *i; i++)
			print_decl(*i, PDECL_INDENT
					| PDECL_NEWLINE
					| PDECL_SYM_OFFSET
					| PDECL_PISDEF
					| PDECL_PINIT
					| PDECL_ATTR);

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

	if(stmt_kind(t, code) && t->symtab && has_st_en_tdef(t->symtab)){
		idt_printf("structs, enums and tdefs in this block:\n");
		gen_str_indent++;
		print_st_en_tdef(t->symtab);
		gen_str_indent--;
	}

	if(t->decls){
		decl **iter;

		idt_printf("stack space %d\n", t->symtab->auto_total_size);
		idt_printf("decls:\n");

		for(iter = t->symtab->decls; *iter; iter++){
			decl *d = *iter;

			gen_str_indent++;
			print_decl(d, PDECL_INDENT
					| PDECL_NEWLINE
					| PDECL_SYM_OFFSET
					| PDECL_PISDEF
					| PDECL_ATTR
					| PDECL_PINIT);
			gen_str_indent--;
		}
	}

	if(t->codes || t->inits){
		stmt **iter;
		int done_code;

		idt_printf("inits and code:\n");

		FOR_INIT_AND_CODE(iter, t, done_code,
			gen_str_indent++;
			print_stmt(*iter);
			gen_str_indent--;
		)
	}
}

void gen_str(symtable *symtab)
{
	decl **diter;

	print_st_en_tdef(symtab);

	for(diter = symtab->decls; diter && *diter; diter++){
		decl *const d = *diter;

		print_decl(d, PDECL_INDENT
				| PDECL_NEWLINE
				| PDECL_PISDEF
				| PDECL_FUNC_DESCEND
				| PDECL_SIZE
				| PDECL_PINIT
				| PDECL_ATTR);
	}
}
