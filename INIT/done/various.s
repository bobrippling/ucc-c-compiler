.section __TEXT,__text
.globl f
f:
	pushq %rbp
	movq %rsp, %rbp
	subq $8, %rsp
	movb $0, %al
	callq g
	movl %eax, -8(%rbp)
	// end of expr-stmt
	movb $0, %al
	callq h
	movl %eax, -4(%rbp)
	// end of expr-stmt
.Lf.1:
	leaveq
	retq
.globl x
.globl a1
.globl a2
.globl a5
.globl a6
.globl a7
.section __DATA,__data
.align 4
x:
.long 2
.long 3
.space 4 # null scalar init
.long 9

.align 4
a1:
.long 1
.long 2
.long 3
.long 4
.long 5
.long 6

.align 4
a2:
.long 1
.long 2
.long 3
.long 4
.long 5
.long 6

.align 4
a5:
.long 1
.long 2
.long 3
.long 4
.long 5
.long 6

.align 4
a6:
.space 4 # null scalar init
.long 100
.space 4 # null scalar init

.align 4
a7:
.long 1
.space 4 # null scalar init
.space 4 # null scalar init
.space 4 # null scalar init
.long 1
.space 4 # null scalar init
.space 4 # null scalar init
.space 4 # null scalar init
.long 1

.section __BSS,__bss
