.globl assert
assert:
	cmp r0, #0
	bxne lr
	b abort
