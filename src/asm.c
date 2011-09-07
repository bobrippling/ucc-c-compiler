#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "tree.h"
#include "sym.h"
#include "asm.h"
#include "tree.h"
#include "platform.h"
#include "alloc.h"

static int label_last = 1, str_last = 1;

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
}

void asm_new(enum asm_type t, void *p)
{
	switch(t){
		case asm_assign:
			printf("pop rax");
			break;

		case asm_call:
			printf("call %s\n", (const char *)p);
			break;

		case asm_load_ident:
			printf("load %s\n", (const char *)p);
			break;

		case asm_load_val:
			printf("load val %d\n", *(int *)p);
			break;

		case asm_op:
			printf("%s\n", op_to_str(*(enum op_type *)p));
			break;

		case asm_pop:
			printf("pop\n");
			break;

		case asm_push:
			printf("push\n");
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

void asm_declare_str(const char *lbl, const char *str)
{
	printf("%s db ", lbl);

	for(; *str; str++)
		printf("%d, %s", *str, str[1] ? "" : "0\n");
}

void asm_temp(const char *fmt, ...)
{
	va_list l;
	va_start(l, fmt);
	vprintf(fmt, l);
	va_end(l);
	putchar('\n');
}
