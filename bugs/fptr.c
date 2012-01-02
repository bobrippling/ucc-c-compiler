int plus(int a, int b)
{
	return a + b;
}

int minus(int a, int b)
{
	return a - b;
}

#ifdef USE_RETURN
int (*getptr(char choice))(int, int)
{
   return choice == '+' ? &plus : &minus;
}
#endif

//typedef int (*fptr)(int, int)

main()
{
	char ch;
	int a, b;
	int (*f)(int, int);

	ch = '-';
	a = 3;
	b = 2;
#ifdef USE_RETURN
	f = getptr(ch);
#else
	f = plus;
#endif

	printf("%d %c %d = %d\n", a, ch, b, f(a, b));
}
