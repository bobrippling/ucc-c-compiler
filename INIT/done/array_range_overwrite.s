.section __TEXT,__text
.globl main
main:
	pushq %rbp
	movq %rsp, %rbp
	subq $24, %rsp
	movl $5, -24(%rbp)
	movl $5, -20(%rbp)
	movl $5, -16(%rbp)
	movl $5, -4(%rbp)
	// end of expr-stmt
	movl $1, -12(%rbp)
	// end of expr-stmt
	movl $2, -8(%rbp)
	// end of expr-stmt
.Lmain.1:
	leaveq
	retq
.section __DATA,__data
.section __BSS,__bss
