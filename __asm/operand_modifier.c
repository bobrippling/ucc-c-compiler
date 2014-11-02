main()
{
	__asm("lea %1(%2,%3,1<<%4),%0"
			: "=r" (out)
			: "%i" (in1), "r" (in2), "r" (in3), "M" (logscale));

	/* this looks like a way to generate a legal lea instruction with all the
	 * possible bells and whistles. There's only one problem. When GCC
	 * substitutes the immediates in1 and logscale, it's going to produce
	 * something like:
   *
	 * lea $-44(%ebx,%eax,1<<$2),%ecx
   *
	 * which is a syntax error. The $ on the constants are not useful in this
	 * context. So there are modifier characters. The one applicable in this
	 * context is 'c', which means to omit the usual immediate value information.
	 * The correct __asm is
	 */

	__asm("lea %c1(%2,%3,1<<%c4),%0"
			: "=r" (out)
			: "%i" (in1), "r" (in2), "r" (in3), "M" (logscale));

	/*
	 * which will produce
	 *
	 * lea -44(%ebx,%eax,1<<2),%ecx
	 */
}

/* There are a few others mentioned in the GCC manual as generic:
 *
 * %c0 substitutes the immediate value %0, but without the immediate syntax.
 * %n0 substitutes like %c0, but the negated value.
 * %l0 substitutes like %c0, but with the syntax expected of a jump target.
 * (This is usually the same as %c0.)
 *
 * And then there are the x86-specific ones. These are, unfortunately, only
 * listed in the i386.h header file in the GCC source (config/i386/i386.h), so
 * you have to dig a bit for them.
 *
 * %q0 prints the 64-bit form of an operand. %rax, etc.
 * %k0 prints the 32-bit form of an operand. %eax, etc.
 * %w0 prints the 16-bit form of an operand. %ax, etc.
 * %b0 prints the 8-bit form of an operand. %al, etc.
 * %h0 prints the high 8-bit form of a register. %ah, etc.
 * %z0 print opcode suffix corresponding to the operand type, b, w or l.
 */
