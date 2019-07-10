// RUN: %ocheck 0 %s
int i = 5;

g(int a, int b, int c)
{
	return b;
}

p(void)
{
	i++;
}

q(a, b, c, d, e, f, g, h, j)
{
	return a + b + c + d + e + f + g + h + j;
}

main()
{
	int (*f)() = p;
	if(q(1, 2, 3, 4, 5, 6, 7, 8, 9) != 45){
		_Noreturn void abort();
		abort();
	}

	f(1);
	f(1 == 2);
	f(5 - 3);

	if(i != 8){
		_Noreturn void abort();
		abort();
	}
	if(g(1, 2, 3) != 2){
		_Noreturn void abort();
		abort();
	}

	return 0;
}
