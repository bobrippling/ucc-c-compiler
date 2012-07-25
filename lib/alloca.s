.section .text
	.globl alloca

alloca:
	# stack looks like:

	# vars, etc...  <-- rsp
	# etc...
	# amount_to_alloca
	# return addr (continue here) <-- rbp

	popq %rdi # return addr
	popq %rsi # amount to alloc

	subq %rsp, %rsi
	movq %rax, %rsp # return

	jmp *%rdi
