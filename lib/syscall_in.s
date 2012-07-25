.section .bss,""
	.globl errno
errno:
	.long 0

.section .text,""
	.globl __syscall
__syscall:
	pushq %rbp
	movq  %rsp, %rbp

	// x64 convention: %rdi, %rsi, %rdx, %rcx, %r8, %r9
	// linux kernel:   %rdi, %rsi, %rdx, %r10, %r8, %r9
	// %rcx and %r11 are destroyed - should save... eh

	movq 16(%rbp), %rax # eax
	movq 24(%rbp), %rdi # ebx
	movq 32(%rbp), %rsi # ecx
	movq 40(%rbp), %rdx # edx
	movq 48(%rbp), %r10 # edi
	movq 56(%rbp), %r8  # esi
	movq 64(%rbp), %r9  # e8?
	syscall
#include "syscall_err.s"
	movl %eax, errno(%rip)
	movq $-1, %rax
.fin:
	leaveq
	retq
