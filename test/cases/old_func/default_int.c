// RUN: %ucc -fsyntax-only %s

f((int));

g(int);

h(a, b, c)
	char *b;
{
}

main()
{
	f(g);
	h(1, 2, 3);
}
