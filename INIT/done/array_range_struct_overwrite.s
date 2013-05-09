.section __TEXT,__text
.globl main
main:
	pushq %rbp
	movq %rsp, %rbp
	subq $56, %rsp
	movl $5, -48(%rbp)
	// end of expr-stmt
	movl $6, -44(%rbp)
	// end of expr-stmt
	movq -48(%rbp), %rax
	movq %rax, -40(%rbp)
	// end of expr-stmt
	movq -48(%rbp), %rax
	movq %rax, -32(%rbp)
	// end of expr-stmt
	movl $1, -24(%rbp)
	// end of expr-stmt
	movl $0, -20(%rbp)
	// end of expr-stmt
	movl $3, -16(%rbp)
	// end of expr-stmt
	movl $4, -12(%rbp)
	// end of expr-stmt
	movq -48(%rbp), %rax
	movq %rax, -8(%rbp)
	// end of expr-stmt
	movl $0, -52(%rbp)
	// end of expr-stmt
	// end of ({...})
	// for-init
.Lflow_for_test_3:
	movl -52(%rbp), %eax
	cmpl $6, %eax
	jge .Lflow_for_start_1
	movl -52(%rbp), %eax
	movsx %eax, %rax
	imulq $8, %rax
	leaq -48(%rbp), %rbx
	addq %rax, %rbx
	addq $4, %rbx
	movl (%rbx), %eax
	// val from struct/union
	movl -52(%rbp), %ebx
	movsx %ebx, %rbx
	imulq $8, %rbx
	leaq -48(%rbp), %rcx
	addq %rbx, %rcx
	movl (%rcx), %ebx
	// val from struct/union
	movl -52(%rbp), %ecx
	leaq str.1(%rip), %rdi
	movl %ecx, %esi
	movl %ebx, %edx
	movl %eax, %ecx
	movb $0, %al
	callq printf
	// end of expr-stmt
.Lflow_for_contiune_2:
	movl -52(%rbp), %eax
	// saved for compound op
	movl -52(%rbp), %ebx
	addl $1, %ebx
	movl %ebx, -52(%rbp)
	// unused for inc
	jmp .Lflow_for_test_3
.Lflow_for_start_1:
.Lmain.1:
	leaveq
	retq
.section __DATA,__data
.align 1
str.1:
.byte 91
.byte 37
.byte 100
.byte 93
.byte 32
.byte 61
.byte 32
.byte 123
.byte 32
.byte 37
.byte 100
.byte 44
.byte 32
.byte 37
.byte 100
.byte 32
.byte 125
.byte 10
.byte 0

.section __BSS,__bss
