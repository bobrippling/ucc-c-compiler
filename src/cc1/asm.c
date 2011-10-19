#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "../util/util.h"
#include "tree.h"
#include "sym.h"
#include "asm.h"
#include "../util/platform.h"
#include "../util/alloc.h"

static int label_last = 1, str_last = 1;
extern FILE *cc1_out;

char *label_code(const char *fmt)
{
	int len;
	char *ret;

	len = strlen(fmt) + 10;
	ret = umalloc(len + 1);

	snprintf(ret, len, ".%s_%d", fmt, label_last++);

	return ret;
}

char *label_str()
{
	char *ret = umalloc(8);
	snprintf(ret, 8, "str_%d", str_last++);
	return ret;
}

void asm_sym(enum asm_sym_type t, sym *s, const char *reg)
{
	switch(s->type){
		case sym_auto:
		case sym_arg:
		{
			int is_auto = s->type == sym_auto;
			char brackets[16];

			snprintf(brackets, sizeof brackets, "[rbp %c %d]",
					is_auto ? '-' : '+',
					((is_auto ? 1 : 2) * platform_word_size()) + s->offset);

			asm_temp("%s %s, %s ; %s%s",
					t == ASM_LEA ? "lea"    : "mov",
					t == ASM_SET ? brackets : reg,
					t == ASM_SET ? reg      : brackets,
					t == ASM_LEA ? "&"      : "",
					s->decl->spel
					);

			break;
		}

		case sym_global:
			fprintf(stderr, "TODO: sym_global command on %s\n", s->decl->spel);
			break;

		case sym_str:
		case sym_func:
			DIE_ICE();
	}
}

void asm_new(enum asm_type t, void *p)
{
	switch(t){
		case asm_assign:
			asm_temp("pop rax");
			break;

		case asm_call:
			asm_temp("call %s", (const char *)p);
			break;

		case asm_load_ident:
			asm_temp("load %s", (const char *)p);
			break;

		case asm_load_val:
			asm_temp("load val %d", *(int *)p);
			break;

		case asm_op:
			asm_temp("%s", op_to_str(*(enum op_type *)p));
			break;

		case asm_pop:
			asm_temp("pop");
			break;

		case asm_push:
			asm_temp("push");
			break;

		case asm_addrof:
			fprintf(stderr, "BUH?? (addrof)\n");
			break;
	}
}

void asm_label(const char *lbl)
{
	asm_temp("%s:", lbl);
}

void asm_declare_str(const char *lbl, const char *str, int len)
{
	int i;

	fprintf(cc1_out, "%s db ", lbl);

	for(i = 0; i < len; i++)
		fprintf(cc1_out, "%d%s", str[i], i == len-1 ? "" : ", ");
	fputc('\n', cc1_out);
}

void asm_temp(const char *fmt, ...)
{
	va_list l;
	va_start(l, fmt);
	vfprintf(cc1_out, fmt, l);
	va_end(l);
	fputc('\n', cc1_out);
}
