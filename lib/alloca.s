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

	subq %rsi, %rsp
	movq %rsp, %rax # return

	jmp *%rdi
