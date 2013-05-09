main()
{
	extern a(void), b(void), c(void);

	a() & b() >= c();

}
#if 0
.section .text
.globl main
main:
	pushq %rbp
	movq %rsp, %rbp
	callq a
	// pre-call reg-save
	subq $0x8, %rsp
	movl %eax, -0x8(%rbp)   // fine, a() on stack
	callq b
	// pre-call reg-save
	subq $0x8, %rsp
	movl %eax, -0x8(%rbp)   // overridden a() with b()
	callq c
	movl -0x8(%rbp), %ebx   // reads b()
	cmpl %ebx, %eax
	movl -0x8(%rbp), %eax   // reads b()
	// zero for cmp
	movl $0, %ebx
	setge %bl
	andl %ebx, %eax
	// end of expr-stmt
.Lmain.1:
	leaveq
	retq
.section .data
.section .bss
#endif
