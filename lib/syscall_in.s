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

	movq %rax, 16(%rbp) # eax
	movq %rdi, 24(%rbp) # ebx
	movq %rsi, 32(%rbp) # ecx
	movq %rdx, 40(%rbp) # edx
	movq %r10, 48(%rbp) # edi
	movq %r8,  56(%rbp) # esi
	movq %r9,  64(%rbp) # e8?
	syscall
#include "syscall_err.s"
	movl %eax, errno(%rip)
	movq %rax, -1
.fin:
	leaveq
	retq
