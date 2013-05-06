.section .text
.globl main
main:
	push %ebp
	movl %esp, %ebp
	subl $0xc, %esp

	movb $104, -0x8(%ebp)
	movb $105, -0x7(%ebp)
	movb $32, -0x6(%ebp)
	movb $0, -0x4(%ebp)
	movl $0, -0xc(%ebp)

.Lflow_for_test_3:
	movl -0xc(%ebp), %eax
	cmpl $10, %eax
	jge .Lflow_for_start_1

	movl -0xc(%ebp), %eax
	addl $48, %eax
	// truncate cast feom int to char, size 4 -> 1
	movb %al, -0x5(%ebp)

	pushl $0
	pushl $str.1

	leal -0x8(%ebp), %eax
	pushl %eax

	pushl $0
	calll _MessageBoxA
	//addl $0x10, %esp - callee cleanup

.Lflow_for_contiune_2:
	incl -0xc(%ebp)

	jmp .Lflow_for_test_3

.Lflow_for_start_1:
	movl $5, %eax

	leavel
	retl

.section .data
str.1:
.byte 67, 97, 112, 116, 105, 111, 110, 0
