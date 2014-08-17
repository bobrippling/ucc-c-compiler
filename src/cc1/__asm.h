#ifndef UCC__ASM_H
#define UCC__ASM_H

typedef struct asm_param
{
	struct expr *exp;
	char *constraints;
	int is_output;
} asm_param;

typedef struct asm_args
{
	asm_param **params;
	char **clobbers;
	char **jumps; /* asm goto */
	char *cmd;
	int extended; /* is it asm("") or asm("":::) */
	int is_volatile;
} asm_args;

#endif
