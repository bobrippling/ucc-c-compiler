f(a, b);

void (*p)(x, y) = 0;

f(a, b)
	int a, b;
{
	return a + b;
}

q(a, b)
	int a;
{
}

z(p, i)
	char *i;
{
}

main(argc, argv)
	int argc;
	char **argv;
{
	return f(2, -1 - 1);
}
