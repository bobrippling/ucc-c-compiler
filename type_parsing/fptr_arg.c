int plus(int a, int b)
{
	return a + b;
}

int minus(int a, int b)
{
	return a - b;
}

int (*getptr(char ch))()
{
	if(ch == '+')
		return plus;
	return &minus;
}

int main(int argc, char **argv)
{
	char ch;
	int a, b;
	int (*f)(int, int);

	if(argc != 2 || argv[1][1]){
		printf("bad input\n");
		return 1;
	}

	a = 3;
	b = 2;
	f = getptr(argv[1][0]);

	printf("%d %c %d = %d\n", a, ch, b, f(a, b));
	return 0;
}
