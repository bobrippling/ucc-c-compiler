typedef struct A A;

struct A {
	long pad1, pad2, pad3;
	int csr;
	int cw;
	long rsp, rbp, rbx, r12, r13, r14, r15;
};

void f(struct A *p)
{
	__asm(
		"stmxcsr   24(%0)\n"
		"fstcw     28(%0)\n"
		"mov %%rsp, 32(%0)\n"
		"mov %%rbp, 40(%0)\n"
		"mov %%rbx, 48(%0)\n"
		"mov %%r12, 56(%0)\n"
		"mov %%r13, 64(%0)\n"
		"mov %%r14, 72(%0)\n"
		"mov %%r15, 80(%0)\n"
		: : "r"(p)
	);
}

void g(struct A *p)
{
	// https://gcc.gnu.org/onlinedocs/gcc/Local-Register-Variables.html
	register long rsp __asm__("rsp");
	register long rbp __asm__("rbp");
	register long rbx __asm__("rbx");
	register long r12 __asm__("r12");
	register long r13 __asm__("r13");
	register long r14 __asm__("r14");
	register long r15 __asm__("r15");

	__asm(
			"stmxcsr   %[csr]\n"
			"fstcw     %[cw]\n"
			/*"mov %%rsp, %[rsp]\n"
			"mov %%rbp, %[rbp]\n"
			"mov %%rbx, %[rbx]\n"
			"mov %%r12, %[r12]\n"
			"mov %%r13, %[r13]\n"
			"mov %%r14, %[r14]\n"
			"mov %%r15, %[r15]\n"*/
			: [csr] "=m"(p->csr)
			, [cw] "=m"(p->cw)
			, [rsp] "=r"(rsp) /* need these intermediates to force use of these registers */
			, [rbp] "=r"(rbp)
			, [rbx] "=r"(rbx)
			, [r12] "=r"(r12)
			, [r13] "=r"(r13)
			, [r14] "=r"(r14)
			, [r15] "=r"(r15)
		);

	p->rsp = rsp;
	p->rbp = rbp;
	p->rbx = rbx;
	p->r12 = r12;
	p->r13 = r13;
	p->r14 = r14;
	p->r15 = r15;
}
