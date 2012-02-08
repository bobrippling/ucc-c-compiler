#include <stdio.h>

#include "ucc.h"

// TODO: enum
#define RAX 0
#define RBX 1
#define RCX 2
#define RDX 3

#define RDI 4
#define RSI 5

#define R8  6
#define R9  7
#define R10 8
#define R11 9

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
#define get_reg(r) __asm__("mov rax, " r)
	unsigned int regs[10];
	unsigned int *rp; // arraysizes workaround
	int i;

	//__asm__("push rax\npush rbx\npush rcx\n");

	rp = regs;

	rp[RAX] = get_reg("rax");
	rp[RBX] = get_reg("rbx");
	rp[RCX] = get_reg("rcx");
//#ifdef FULL
	rp[RDX] = get_reg("rdx");

	rp[RDI] = get_reg("rdi");
	rp[RSI] = get_reg("rsi");

	rp[R8 ] = get_reg("r8 ");
	rp[R9 ] = get_reg("r9 ");
	rp[R10] = get_reg("r10");
	rp[R11] = get_reg("r11");

	for(i = 0; i < 10; i++)
		printf("register[%d] = %s = 0x%x (%d)\n",
				i, regnames[i], rp[i], rp[i]);
//#endif
}

/*
int main()
{
	__asm__("mov rax, 42");
	__asm__("mov rbx, 42");
	__asm__("mov rcx, 42");
	__asm__("mov rdx, 42");
	__asm__("mov rdi, 42");
	__asm__("mov rsi, 42");
	__asm__("mov r8 , 42");
	__asm__("mov r9 , 42");
	__asm__("mov r10, 42");
	__asm__("mov r11, 42");

	__dump_regs();

	printf("back in main\n");
	return 0;
}
*/
