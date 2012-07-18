.globl exit
exit:
	movq $60, %rax
	movq 8(%rsp), %rdi
	syscall
	hlt
