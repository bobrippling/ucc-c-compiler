// inline
int f(int x)
{
	return x + 1;
}

int h(int);

// don't inline
int g(int x)
{
	int t = x;
	for(int i = 0; i < 10; i++)
		t += h(i);

	t++;

	return t * x;
}

int main()
{
	printf("%d\n", f(5));
	printf("%d\n", g(5));
}
