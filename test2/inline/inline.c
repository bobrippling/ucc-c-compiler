// RUN: %inline_check %s

inline int f(int i)
{
	return 3 + i;
}

char *g(int x)
{
	return "hello" + x;
}

int main()
{
	printf("%s\n", g(1));

	return f(2);
}
