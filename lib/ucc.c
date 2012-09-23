#include "stdio.h"
#include "stdlib.h"

#include "ucc.h"

enum reg
{
	RAX,
	RBX,
	RCX,
	RDX,

	RDI,
	RSI,

	R8,
	R9,
	R10,
	R11,
};

static const char *regnames[] = {
	"rax",
	"rbx",
	"rcx",
	"rdx",

	"rdi",
	"rsi",

	"r8 ",
	"r9 ",
	"r10",
	"r11",
};

void __dump_regs()
{
	abort();
	/*
#define GET_REG(dest, src) \
	asm("mov %0, " src : : "" (dest))


	unsigned int regs[10];
	int i;

	get_reg("rax", RAX);
	get_reg("rbx", RBX);
	get_reg("rcx", RCX);

	for(i = 0; i < 10; i++)
		printf("register[%d] = %s = 0x%x (%d)\n",
				i, regnames[i], rp[i], rp[i]);
	*/
}
