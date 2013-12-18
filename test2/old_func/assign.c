// RUN: %check %s

f(a)
	char *a;
{
	return 0;
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

	ifp = f; // CHECK: !/warn/
	cpfp = h; // CHECK: !/warn/

	cpfp = f; // CHECK: warning: mismatching types, assignment:
	ifp = h; // CHECK: warning: mismatching types, assignment:
}
