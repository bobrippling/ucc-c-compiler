// RUN: %ocheck 28 %s

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

	return 28;
}
