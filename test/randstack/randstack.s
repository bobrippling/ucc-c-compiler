.globl get_rsp
.globl _get_rsp
get_rsp:
_get_rsp:
	mov %rsp, %rax
	ret

#.globl randomise_stack_asm
#randomise_stack_asm:
#	mov %rsp, %rsi
#
#	leaq -512(%rsp), %rdi
#	xor %eax, %eax
#	dec %rax
#1:
#	cmp %rsp, %rdi
#	jge 1f
#	push %rax
#	jmp 1b
#1:
#
#	mov %rsi, %rsp
#	ret
