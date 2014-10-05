.globl check_align
check_align:

	# simulate a stack entry
	sub $8, %rsp

	mov %rsp, %rax

	# bottom 15 bits should be zero
	and $0xf, %rax
	cmp $0, %rax
	je Lfine

	ud2

Lfine:
	add $8, %rsp
	ret
