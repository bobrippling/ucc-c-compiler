#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

#include "../util/util.h"
#include "data_structs.h"
#include "cc1.h"
#include "sym.h"
#include "asm.h"
#include "../util/platform.h"
#include "../util/alloc.h"
#include "sue.h"
#include "const.h"

#define SNPRINTF(s, n, ...) \
		UCC_ASSERT(snprintf(s, n, __VA_ARGS__) != n, "snprintf buffer too small")

static int label_last    = 1,
					 str_last      = 1,
					 switch_last   = 1,
					 flow_last     = 1,
					 block_last    = 1;

char *asm_label_block(const char *funcsp)
{
	int len;
	char *ret;

	len = strlen(funcsp) + 16;
	ret = umalloc(len);

	SNPRINTF(ret, len, "%s.block_%d", funcsp, block_last++);

	return ret;
}

char *asm_label_code(const char *fmt)
{
	int len;
	char *ret;

	len = strlen(fmt) + 10;
	ret = umalloc(len + 1);

	SNPRINTF(ret, len, ".%s.%d", fmt, label_last++);

	return ret;
}

char *asm_label_array(int str)
{
	char *ret = umalloc(16);
	SNPRINTF(ret, 16, "%s.%d", str ? "str" : "array", str_last++);
	return ret;
}

char *asm_label_static_local(const char *funcsp, const char *spel)
{
	char *ret;
	int len;

	UCC_ASSERT(funcsp, "no spel for %s", __func__);

	len = strlen(funcsp) + strlen(spel) + 9;
	ret = umalloc(len);
	SNPRINTF(ret, len, "%s.static_%s", funcsp, spel);
	return ret;
}

char *asm_label_goto(char *lbl)
{
	int len = strlen(lbl) + 6;
	char *ret = umalloc(len);
	SNPRINTF(ret, len, ".lbl_%s", lbl);
	return ret;
}

char *asm_label_case(enum asm_label_type lbltype, int val)
{
	int len;
	char *ret = umalloc(len = 15 + 32);
	switch(lbltype){
		case CASE_DEF:
			SNPRINTF(ret, len, ".case_%d_default", switch_last);
			break;

		case CASE_CASE:
		case CASE_RANGE:
		{
			const char *extra = "";
			if(val < 0){
				val = -val;
				extra = "m";
			}
			SNPRINTF(ret, len, ".case%s_%d_%s%d", lbltype == CASE_RANGE ? "_rng" : "", switch_last, extra, val);
			break;
		}
	}
	switch_last++;
	return ret;

}

char *asm_label_flow(const char *fmt)
{
	int len = 16 + strlen(fmt);
	char *ret = umalloc(len);
	SNPRINTF(ret, len, ".flow_%s_%d", fmt, flow_last++);
	return ret;
}

void asm_sym(enum asm_sym_type t, sym *s, const char *reg)
{
	const int is_global = s->type == sym_global || type_store_static_or_extern(s->decl->type->store);
	char *const dsp = s->decl->spel;
	int is_auto = s->type == sym_local;
	char  stackbrackets[16];
	char *brackets;

	if(is_global){
		const int bracket_len = strlen(dsp) + 16;

		brackets = umalloc(bracket_len + 1);

		if(t == ASM_LEA || s->decl->func_code){
			SNPRINTF(brackets, bracket_len, "%s", dsp); /* int (*p)() = printf; for example */
			/*
			 * either:
			 *   we want             lea rax, [a]
			 *   and convert this to mov rax, a   // this is because Macho-64 is an awful binary format
			 * force a mov for funcs (i.e. &func == func)
			 */
			t = ASM_LOAD;
		}else{
			const char *type_s = "";
			enum asm_size ts = asm_type_size(s->decl);

			if(ts == ASM_SIZE_WORD)
				type_s = "qword ";
			else if(ts == ASM_SIZE_STRUCT_UNION)
				ICE("operation on full %s", sue_str(s->decl->type->sue));

			/* get warnings for "lea rax, [qword tim]", just do "lea rax, [tim]" */
			SNPRINTF(brackets, bracket_len, "[%s%s]",
					t == ASM_LEA ? "" : type_s, dsp);
		}
	}else{
		brackets = stackbrackets;
		SNPRINTF(brackets, sizeof stackbrackets, "[rbp %c %d]",
				is_auto ? '-' : '+',
				((is_auto ? 1 : 2) * platform_word_size()) + s->offset);
	}

	asm_temp(1, "%s %s, %s ; %s%s",
			t == ASM_LEA ? "lea"    : "mov",
			t == ASM_SET ? brackets : reg,
			t == ASM_SET ? reg      : brackets,
			t == ASM_LEA ? "&"      : "",
			dsp
			);

	if(brackets != stackbrackets)
		free(brackets);
}

void asm_indir(enum asm_indir mode, decl *tt, char rto, char rfrom, const char *comment)
{
	const int full_word = asm_type_size(tt) == ASM_SIZE_WORD;

	if(mode == ASM_INDIR_GET){
		if(full_word)
			asm_temp(1, "mov r%cx, [r%cx]", rto, rfrom);
		else
			asm_temp(1, "movzx r%cx, byte [r%cx]", rto, rfrom);
	}else{
		if(full_word)
			asm_temp(1, "mov [r%cx], r%cx", rto, rfrom);
		else
			asm_temp(1, "mov byte [r%cx], %cl", rto, rfrom);
	}

	if(comment)
		asm_temp(1, "; %s", comment);
}

void asm_new(enum asm_type t, void *p)
{
	switch(t){
		case asm_assign:
			asm_temp(1, "pop rax");
			break;

		case asm_call:
			asm_temp(1, "call %s", (const char *)p);
			break;

		case asm_load_ident:
			asm_temp(1, "load %s", (const char *)p);
			break;

		case asm_load_val:
			asm_temp(1, "load val %d", *(int *)p);
			break;

		case asm_op:
			asm_temp(1, "%s", op_to_str(*(enum op_type *)p));
			break;

		case asm_pop:
			asm_temp(1, "pop");
			break;

		case asm_push:
			asm_temp(1, "push");
			break;

		case asm_addrof:
			fprintf(stderr, "BUH?? (addrof)\n");
			break;
	}
}

void asm_label(const char *lbl)
{
	asm_temp(0, "%s:", lbl);
}

void asm_out_intval(FILE *f, intval *iv)
{
	char fmt[4];
	char *p = fmt;

	*p++ = '%';
	if(iv->suffix & VAL_LONG)
		*p++ = 'l';

	strcpy(p, iv->suffix & VAL_UNSIGNED ? "u" : "d");

	fprintf(f, fmt, iv->val);
}

void asm_declare_single_part(FILE *f, expr *e)
{
	if(!e->f_gen_1)
		ICE("unexpected global initaliser %s (no gen_1())", e->f_str());

	e->f_gen_1(e, f);
}

enum asm_size asm_type_size(decl *d)
{
	if(decl_desc_depth(d)){
		return ASM_SIZE_WORD;
	}else{
		if(d->type->typeof)
			ICE("typedefs should've been folded by now");

		switch(d->type->primitive){
			case type_enum:
			case type_int:
				return ASM_SIZE_WORD;

			case type_char:
				return ASM_SIZE_1;

			case type_void:
				ICE("type primitive is void");

			case type_struct:
			case type_union:
				return ASM_SIZE_STRUCT_UNION;

			case type_unknown:
				ICE("type primitive not set");
		}
	}

	ICE("asm_type_size switch error");
	return ASM_SIZE_WORD;
}

char asm_type_ch(decl *d)
{
	return asm_type_size(d) == ASM_SIZE_WORD ? 'q' : 'b';
}

void asm_declare_single(FILE *f, decl *d)
{
	if(asm_type_size(d) == ASM_SIZE_STRUCT_UNION){
		/* struct init */
		int i;

		ICW("attempting to declare+init a struct, untested for nesting and arrays");

		UCC_ASSERT(d->init->array_store, "no array store for struct init (TODO?)");
		UCC_ASSERT(d->init->array_store->type == array_exprs, "array store of strings for struct");

		fprintf(f, "%s dq ", d->spel); /* XXX: assumes all struct members are word-size */

		for(i = 0; i < d->init->array_store->len; i++){
			intval iv;

			const_fold_need_val(d->init->array_store->data.exprs[i], &iv);

			fprintf(f, "%ld%s",
					iv.val,
					i == d->init->array_store->len - 1 ? "" : ", ");
		}

	}else{
		fprintf(f, "%s d%c ", d->spel, asm_type_ch(d));

		asm_declare_single_part(f, d->init);
	}

	fputc('\n', f);
}

void asm_declare_array(enum section_type output, const char *lbl, array_decl *ad)
{
	int i;

	fprintf(cc_out[output], "%s d%c ", lbl, ad->type == array_str ? 'b' : 'q');

	for(i = 0; i < ad->len; i++){
		if(ad->type == array_str)
			fprintf(cc_out[output], "%d", ad->data.str[i]);
		else
			asm_declare_single_part(cc_out[output], ad->data.exprs[i]);

		if(i < ad->len - 1)
			fputs(", ", cc_out[output]);
	}

	fputc('\n', cc_out[output]);
}

void asm_tempfv(FILE *f, int indent, const char *fmt, va_list l)
{
	if(indent)
		fputc('\t', f);

	vfprintf(f, fmt, l);

	fputc('\n', f);
}

void asm_temp(int indent, const char *fmt, ...)
{
	va_list l;
	va_start(l, fmt);
	asm_tempfv(cc_out[SECTION_TEXT], indent, fmt, l);
	va_end(l);
}

void asm_tempf(FILE *f, int indent, const char *fmt, ...)
{
	va_list l;
	va_start(l, fmt);
	asm_tempfv(f, indent, fmt, l);
	va_end(l);
}
