#include "../../../src/as_cfg.h"
#include "../defs.h"
.section SECTION_BSS
	.globl SYMBL(errno)
SYMBL(errno):
	.long 0

.section SECTION_TEXT
	.globl SYMBL(__syscall)
SYMBL(__syscall):
	pushq %rbp
	movq  %rsp, %rbp

	// x64 convention: %rdi, %rsi, %rdx, %rcx, %r8, %r9
	// linux kernel:   %rdi, %rsi, %rdx, %r10, %r8, %r9
	// %rcx and %r11 are destroyed - should save
	pushq %rcx
	pushq %r11

	// x64 convention: %rdi, %rsi, %rdx, %rcx, %r8, %r9
	// kernel conv.:   %rdi, %rsi, %rdx, %r10, %r8, %r9

	// convert our args to syscall args
	movq %rdi, %rax
	movq %rsi, %rdi
	movq %rdx, %rsi
	movq %rcx, %rdx
	movq %r8 , %r10
	movq %r9 , %r8
	movq 0x8(%rbp), %r9

	syscall
#ifdef __linux__
	cmpq $-1, %rax
	jg .fin
	negq %rax # get positive errno
#else
	jnc .fin
	// carry - err is in rax
#endif
	movl %eax, SYMBL(errno)(%rip)
	movq $-1, %rax
.fin:

	popq %r11
	popq %rcx

	leaveq
	retq
