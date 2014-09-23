g()
{
	return 5;
}

populate(int *p, int n)
{
	while(n --> 0)
		*p++ = n;
}

sum(int *p, int n)
{
	int t = 0;
	while(n --> 0)
		t += *p++;
	return t;
}

f(int n)
{
	// ebx is saved at the bottom of the stack, which should be used for arguments
	int x[n + g()];

	populate(x, sizeof x / sizeof *x);
	return sum(x, sizeof x / sizeof *x);
}

main()
{
	printf("hi %d\n", f(3));
	// argument 3 to f() is at -16 on stack
	// then printf() needs the stack to be -32, so f()'s argument is wrong
}
