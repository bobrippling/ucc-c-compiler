.section .text
	.globl memcpy

memcpy:
	# n
	# src
	# dest
	# retaddr
	movq %rdi, %rbx
	movq %rdx, %rcx # get into the "count" register
	cld
	repnz movsb
	movq %rbx, %rax # return dest
	ret
