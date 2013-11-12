// RUN: %ucc -o %t %s
// RUN: %t | %output_check 1 2 3 4 5 7 3 3

void inst(int i)
{
	printf("%d\n", i);
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
}
