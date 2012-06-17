#include "stdio.h"

#include "ucc.h"

enum reg
{
	RAX,
	RBX,
	RCX,
	RDX,

	RDI,
	RSI,

#ifdef EIGHT_ONWARDS
	R8,
	R9,
	R10,
	R11,
#endif
};

static const char *regnames[] = {
	"rax",
	"rbx",
	"rcx",
	"rdx",

	"rdi",
	"rsi",

#ifdef EIGHT_ONWARDS
	"r8 ",
	"r9 ",
	"r10",
	"r11",
#endif
};

void __dump_regs()
{
#define GET_REG(dest, src) \
	asm("mov %0, " src : : "" (dest))

	unsigned int regs[10];
	unsigned int *rp; // arraysizes workaround
	int i;

	//__asm__("push rax\npush rbx\npush rcx\n");

	rp = regs;

	GET_REG(rp[RAX], "a");
	GET_REG(rp[RBX], "b");
	GET_REG(rp[RCX], "c");
	GET_REG(rp[RDX], "d");

	GET_REG(rp[RDI], "D");
	GET_REG(rp[RSI], "S");

#ifdef FULL
	GET_REG(rp[R8 ], "8" );
	GET_REG(rp[R9 ], "9" );
	GET_REG(rp[R10], "10");
	GET_REG(rp[R11], "11");
#endif

	for(i = 0; i < 10; i++)
		printf("register[%d] = %s = 0x%x (%d)\n",
				i, regnames[i], rp[i], rp[i]);
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

	return 0;
}
*/
