int func11(int p1, int p2, int p3)
{
	asm ("add %1, %0" : "+d" (p1) : "r" (p3) : "cc");

	return p1;
}

/*
The above is a little tricky. p3 is passed in within %edx as specified by the function ABI. This means that gcc needs to copy it into another register so that p1 can go there. Fortunately, gcc handles all of the marshalling for us:


	.p2align 4,,15
	.globl	func11
	.type	func11, @function
func11:
.LFB17:
	.cfi_startproc
	movl	%edx, %ecx
	movl	%edi, %edx
#APP
# 96 "gcc_asm.c" 1
	add %ecx, %edx
# 0 "" 2
#NO_APP
	movl	%edx, %eax
	ret
	.cfi_endproc
.LFE17:
	.size	func11, .-func11
*/
