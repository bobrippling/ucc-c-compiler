.section .data
fp:
.long 5
.long 3
.space 4

.section .rodata
fmt:
.ascii "%d\n"
.zero 1

.section .text
.globl main
main:
	pushq %rbp
	movq %rsp, %rbp

	cvtsi2ss fp,   %xmm0
	cvtsi2ss fp+4, %xmm1

	addss %xmm1, %xmm0
	cvtss2si %xmm0, %eax
	movl %eax, fp+8

	movl 8+fp, %esi
	movq $fmt, %rdi
	call printf

	xorps %xmm0, %xmm0
	cvtss2si %xmm0, %edi
	call exit
	hlt
