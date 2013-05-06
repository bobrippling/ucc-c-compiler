i;

a()
{
	i++;
}

b()
{
	i += 3;
}

get(int d, int (**p)())
{
	if(d)
		*p = a;
	else
		*p = &b;
}

main()
{
	int (*p)();

	get(0, &p);
	p();

	get(1, &p);
	p();

	return i;
}
