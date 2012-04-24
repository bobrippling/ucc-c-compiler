#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

#include "../util/util.h"
#include "data_structs.h"
#include "cc1.h"
#include "sym.h"
#include "asm_out.h"
#include "asm.h"
#include "../util/platform.h"
#include "../util/alloc.h"
#include "sue.h"

static int label_last = 1, str_last = 1, switch_last = 1, flow_last = 1;

static const struct
{
	int sz;
	char ch;
	const char *str;
	const char *regpre, *regpost;
} asm_type_table[] = {
	{ 1,  'b', "byte" , "",  "l"  },
	{ 2,  'w', "word" , "",  "x"  },
	{ 4,  'd', "dword", "e", "x" },
	{ 8,  'q', "qword", "r", "x" },
};

char *asm_label_code(const char *fmt)
{
	int len;
	char *ret;

	len = strlen(fmt) + 10;
	ret = umalloc(len + 1);

	snprintf(ret, len, ".%s.%d", fmt, label_last++);

	return ret;
}

char *asm_label_array(int str)
{
	char *ret = umalloc(16);
	snprintf(ret, 16, "%s.%d", str ? "str" : "array", str_last++);
	return ret;
}

char *asm_label_static_local(const char *funcsp, const char *spel)
{
	char *ret;

	UCC_ASSERT(funcsp, "no spel for %s", __func__);

	ret = umalloc(strlen(funcsp) + strlen(spel) + 9);
	sprintf(ret, "%s.static_%s", funcsp, spel);
	return ret;
}

char *asm_label_goto(char *lbl)
{
	char *ret = umalloc(strlen(lbl) + 6);
	sprintf(ret, ".lbl_%s", lbl);
	return ret;
}

char *asm_label_case(enum asm_label_type lbltype, int val)
{
	char *ret = umalloc(15 + 32);
	switch(lbltype){
		case CASE_DEF:
			sprintf(ret, ".case_%d_default", switch_last);
			break;

		case CASE_CASE:
		case CASE_RANGE:
		{
			const char *extra = "";
			if(val < 0){
				val = -val;
				extra = "m";
			}
			sprintf(ret, ".case%s_%d_%s%d", lbltype == CASE_RANGE ? "_rng" : "", switch_last, extra, val);
			break;
		}
	}
	switch_last++;
	return ret;

}

char *asm_label_flow(const char *fmt)
{
	char *ret = umalloc(16 + strlen(fmt));
	sprintf(ret, ".flow_%s_%d", fmt, flow_last++);
	return ret;
}

void asm_sym(enum asm_sym_type t, sym *s, asm_operand *reg)
{
	const int is_global = s->type == sym_global || type_store_static_or_extern(s->decl->type->store);
	char *const dsp = s->decl->spel;
	int is_auto = s->type == sym_local;
	asm_operand *brackets;

	if(is_global){
		if(t == ASM_LEA || s->decl->func_code){
			brackets = asm_operand_new_label(NULL, dsp);
			/*
			 * either:
			 *   we want             lea rax, [a]
			 *   and convert this to mov rax, a   // this is because Macho-64 is an awful binary format
			 * force a mov for funcs (i.e. &func == func)
			 */
			t = ASM_LOAD;
		}else{
			/* get warnings for "lea rax, [qword tim]", just do "lea rax, [tim]" */
			brackets = asm_operand_new_deref(
					NULL, /* pointer */
					asm_operand_new_label(NULL, dsp),
					0);
		}
	}else{
		brackets = asm_operand_new_deref(
				NULL, /* pointer */
				asm_operand_new_reg(NULL, ASM_REG_BP),
				/*((is_auto ? -1 : 2) * platform_word_size()) + s->offset); - broken? */
				(is_auto ? -1 : 1) * (((is_auto ? 1 : 2) * platform_word_size()) + s->offset));
	}

#if 0
	asm_temp(1, "%s %s, %s ; %s%s",
			t == ASM_LEA ? "lea"    : "mov",
			t == ASM_SET ? brackets : reg,
			t == ASM_SET ? reg      : brackets,
			t == ASM_LEA ? "&"      : "",
			dsp
			);
#endif

	asm_output_new(
			t == ASM_LEA   ? asm_out_type_lea : asm_out_type_mov,
			t == ASM_STORE ? brackets         : reg,
			t == ASM_STORE ? reg              : brackets);

	asm_comment("%s%s", t == ASM_LEA ? "&" : "", dsp);
}

const char *asm_intval_str(intval *iv)
{
	static char buf[32];
	char fmt[4];
	char *p = fmt;

	*p++ = '%';
	if(iv->suffix & VAL_LONG)
		*p++ = 'l';

	strcpy(p, iv->suffix & VAL_UNSIGNED ? "u" : "d");

	snprintf(buf, sizeof buf, fmt, iv->val);
	return buf;
}

void asm_declare_single_part(FILE *f, expr *e)
{
	if(!e->f_gen_1)
		ICE("unexpected global initaliser, expr %s (no gen_1())", e->f_str());

	e->f_gen_1(e, f);
}

int asm_table_lookup(decl *d)
{
	enum
	{
		INDEX_PTR   = 3,
		INDEX_CHAR  = 0,
		INDEX_SHORT = 1,
		INDEX_INT   = 2,
		INDEX_LONG  = 3
	};

	if(!d || decl_ptr_depth(d)){
		return INDEX_PTR;
	}else{
		if(d->type->typeof)
			ICE("typedefs should've been folded by now");

		switch(d->type->primitive){
			case type_void:
				ICE("type primitive is void");

			case type_char:  return INDEX_CHAR;
			case type_short: return INDEX_SHORT;

			case type_enum:
			case type_int:
			case type_float:
				return INDEX_INT;

			case type_double:
			case type_long:
				return INDEX_LONG;

			case type_struct:
			case type_union:
				die_at(&d->where, "invalid use of struct (%s:%d)", __FILE__, __LINE__);

			case type_unknown:
				ICE("type primitive not set");
		}
	}
	ICE("%s switch error", __func__);
	return 0;
}

char asm_type_ch(decl *d)
{
	return asm_type_table[asm_table_lookup(d)].ch;
}

const char *asm_type_str(decl *d)
{
	return asm_type_table[asm_table_lookup(d)].str;
}

void asm_reg_name(decl *d, const char **regpre, const char **regpost)
{
	const int i = asm_table_lookup(d);
	*regpre  = asm_type_table[i].regpre;
	*regpost = asm_type_table[i].regpost;
}

int asm_type_size(decl *d)
{
	struct_union_enum_st *st = d->type->sue;

	if(st && !decl_ptr_depth(d) && st->primitive != type_enum)
		return struct_union_size(st);

	return asm_type_table[asm_table_lookup(d)].sz;
}

void asm_declare_single(FILE *f, decl *d)
{
	if(!decl_ptr_depth(d) && d->type->sue && d->type->sue->primitive != type_enum){
		/* struct init */
		int i;

		ICW("attempting to declare+init a struct, very buggy");

		UCC_ASSERT(d->init->array_store, "no array store for struct init (TODO?)");
		UCC_ASSERT(d->init->array_store->type == array_exprs, "array store of strings for struct");

		fprintf(f, "%s dq ", d->spel); /* XXX: assumes all struct members are word-size */

		for(i = 0; i < d->init->array_store->len; i++)
			fprintf(f, "%ld%s",
					d->init->array_store->data.exprs[i]->val.iv.val,
					i == d->init->array_store->len - 1 ? "" : ", "
					);

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
