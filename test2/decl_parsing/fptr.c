// RUN: %ocheck 0 %s

int t;

void inst(int i)
{
	t += i;
}

main()
{

	void (*f)(int);
	void (**pf)(int);

	inst(1);

	f = inst;
	pf = &f;

	*f;

	f(2);

	(*f)(3);

	(*pf)(4);
	(**pf)(5);
	(*******pf)(7);

	(****inst)(3);
	(&inst)(3);

	extern void _Noreturn abort();
	if(t != 28)
		abort();

	return 0;
}
