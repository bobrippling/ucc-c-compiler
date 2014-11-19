// RUN: %ocheck 0 %s

extern _Noreturn void abort(void);

void f(int i)
{
	if(i != 2)
		abort();
}

void g(a, b, c, d, e, f, g, h)
{
	if(a != 1) abort();
	if(b != 2) abort();
	if(c != 3) abort();
	if(d != 4) abort();
	if(e != 5) abort();
	if(f != 6) abort();
	if(g != 7) abort();
	if(h != 8) abort();
}

void h(a, b, c, d, e, f, g, h, i, j, k)
{
	if(a != 1) abort();
	if(b != 2) abort();
	if(c != 3) abort();
	if(d != 4) abort();
	if(e != 5) abort();
	if(f != 6) abort();
	if(g != 7) abort();
	if(h != 8) abort();
	if(i != 9) abort();
	if(j != 10) abort();
	if(k != 11) abort();
}

q(){ return 5; }
void use(int *p){ (void)p; }

main()
{
	f(2);

	g(1, 2, 3, 4, 5, 6, 7, 8);

	{
		int vla[q()];
		vla[0] = 1;
		vla[1] = 2;
		vla[2] = 3;
		vla[3] = 4;
		vla[4] = 5;

		// in the scope of a vla - need to
		// sub the stack manually
		h(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11);
		// odd/non-pow-16 number of spill args - check alignment
		// i.e. 11 - 6 = 5, 5 * 8 = 40, 40 % 16 != 0

#define EQ(vla, i, e) if(vla[i] != e) abort()
		EQ(vla, 0, 1);
		EQ(vla, 1, 2);
		EQ(vla, 2, 3);
		EQ(vla, 3, 4);
		EQ(vla, 4, 5);

		use(vla);
	}

	// no vla - should be able to just spill the args at (%rsp)
	h(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11);

	return 0;
}
