struct
{
	auto x;
};

main()
{
	for(static int i = 5;;); // error

	for(auto x;;); // fine

	for(struct { int i, j; } *p = (void *)0; // error - can't define types here
			p->i;
			++p->i)
	{
	}
}
