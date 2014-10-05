// RUN: %check -e %s

f(int *p);

g(int n)
{
	typedef int vla[n];

	f(vla); // CHECK: error: use of typedef-name 'vla' as expression

	typedef char ch;

	g(ch); // CHECK: error: use of typedef-name 'ch' as expression
}

main()
{
	g(2);
}
