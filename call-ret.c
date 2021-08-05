struct A {
	char c;
	signed char sc;
	unsigned char uc;
	int i;
};

int f(int a, int b, int c, int d, int e)
{
	g("");
	return a + b + c + d + e;
}

int main()
{
	printf("%d\n", f(1, 2, 3, 4, 5));
}
