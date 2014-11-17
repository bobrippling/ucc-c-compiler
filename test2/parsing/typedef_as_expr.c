// RUN: %check -e %s

f(int *p);

g(int n)
{
	typedef int vla[n];

	f(vla); // CHECK: error: use of typedef-name 'vla' as expression

	typedef char ch;

	g(ch); // CHECK: error: undeclared identifier "ch"
	// non-symbol typedefs don't have symbols, so no 'typedef-name' error
}

main()
{
	g(2);
}
