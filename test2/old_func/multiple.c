// RUN: %ucc %s
// RUN: %ucc %s | %check %s

f(hello); // CHECK: /warning: parameter names without types/

g(a)
{
	return a;
}

main()
{
	return f(5) + g(1);
}

f(a)
	char *a;
{
	return a ? 1 : 0;
}

char *h(a)
	char *a;
{
	return a;
}

test()
{
	int   (*ifp)(char *);
	char *(*cpfp)(char *);

	ifp = f; // fine
	cpfp = h; // fine

	cpfp = f; // not fine
	ifp = h; // not fine
}
