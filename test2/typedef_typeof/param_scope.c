// RUN: %ucc -fsyntax-only %s
main()
{
	int x;
	void f(typeof(x) *);

	f(&x);
}
