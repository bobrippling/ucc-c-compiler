#include <stdio.h>
#include <stdarg.h>

#include "tree.h"
#include "sym.h"
#include "asm.h"
#include "tree.h"
#include "platform.h"

void asm_sym(enum asm_sym_type t, sym *s, const char *reg)
{
	int is_auto = s->type == sym_auto;
	char brackets[16];

	snprintf(brackets, sizeof brackets, "[rbp %c %d]",
			is_auto ? '-' : '+',
			((is_auto ? 1 : 2) * platform_word_size()) + s->offset);

	asm_temp("mov %s, %s ; %s",
			t == ASM_SET ? brackets : reg,
			t == ASM_SET ? reg      : brackets,
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
			printf("call %s\n", p);
			break;

		case asm_load_ident:
			printf("load %s\n", p);
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
			printf("&%s\n", p);
			break;
	}
}

void asm_temp(const char *fmt, ...)
{
	va_list l;
	va_start(l, fmt);
	vprintf(fmt, l);
	va_end(l);
	putchar('\n');
}
