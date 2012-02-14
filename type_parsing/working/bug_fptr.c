int plus(int a, int b)
{
	return a + b;
}

int minus(int a, int b)
{
	return a - b;
}

getptr()
{
   return plus; // &plus
}

main()
{
	char ch;
	int a, b;
	int (*f)(int, int);

	ch = '-';
	a = 3;
	b = 2;
	f = getptr(ch);

	printf("%d %c %d = %d\n", a, ch, b, f(a, b));
}
