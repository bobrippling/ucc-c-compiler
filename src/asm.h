#ifndef ASM_H
#define ASM_H

typedef struct
{
	enum asm_type
	{
		asm_assign,
		asm_call,
		asm_load_ident,
		asm_load_val,
		asm_op,
		asm_pop,
		asm_push,
		asm_addrof
	} type;

} asmop;

enum asm_sym_type
{
	ASM_SET,
	ASM_LOAD,
	ASM_LEA
};

void asm_new(enum asm_type, void *);
void asm_temp(const char *, ...);
void asm_sym(enum asm_sym_type, sym *, const char *);
char *asm_label(const char *fmt);


#endif
