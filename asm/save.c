f(int *p)
{
	asm("movl $5, %0" : "=m"(*p));
	/* here it's possible to inline fully:
	 *
	 * movq -8(%rbp), %rax
	 * movl $5, (%rax)
	 */

	asm("movl $5, %0" : "=r"(*p));
	/* but if the constraint is a register
	 * then we must do a post assignment from the register to *p
	 *
	 * movl $5, %ebx # rhs must be a register due to constraint
	 * movq -8(%rbp), %rax
	 * movl %ebx, (%rax)
	 */
}
