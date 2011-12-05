#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "cc1.h"
#include "../util/util.h"
#include "tree.h"
#include "sym.h"
#include "asm.h"
#include "../util/platform.h"
#include "../util/alloc.h"

static int label_last = 1, str_last = 1;

char *asm_code_label(const char *fmt)
{
	int len;
	char *ret;

	len = strlen(fmt) + 10;
	ret = umalloc(len + 1);

	snprintf(ret, len, ".%s_%d", fmt, label_last++);

	return ret;
}

char *asm_array_label(int str)
{
	char *ret = umalloc(16);
	snprintf(ret, 16, "__%s_%d", str ? "str" : "array", str_last++);
	return ret;
}

void asm_sym(enum asm_sym_type t, sym *s, const char *reg)
{
	switch(s->type){
		case sym_auto:
		case sym_arg:
		case sym_global:
		{
			int is_auto = s->type == sym_auto;
			char brackets[16];

			if(s->type == sym_global){
				const char *type_s = "";

				if(s->decl->ptr_depth || s->decl->type->primitive == type_int)
					type_s = "qword ";

				/* get warnings for "lea rax, [qword tim]", just do "lea rax, [tim]" */
				snprintf(brackets, sizeof brackets, "[%s%s]",
						t == ASM_LEA ? "" : type_s, s->decl->spel);
			}else{
				snprintf(brackets, sizeof brackets, "[rbp %c %d]",
						is_auto ? '-' : '+',
						((is_auto ? 1 : 2) * platform_word_size()) + s->offset);
			}

			asm_temp(1, "%s %s, %s ; %s%s",
					t == ASM_LEA ? "lea"    : "mov",
					t == ASM_SET ? brackets : reg,
					t == ASM_SET ? reg      : brackets,
					t == ASM_LEA ? "&"      : "",
					s->decl->spel
					);

			break;
		}

		case sym_func:
			ICE("asm_sym: can't handle sym_func");
	}
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

void asm_declare_array(enum section_type output, const char *lbl, array_decl *ad)
{
	int i;

	fprintf(cc_out[output], "%s d%c ", lbl, ad->type == array_str ? 'b' : 'q');

	for(i = 0; i < ad->len; i++)
		fprintf(cc_out[output], "%d%s", ad->type == array_str ? ad->data.str[i] : ad->data.exprs[i]->val.i, i == ad->len - 1 ? "" : ", ");

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
