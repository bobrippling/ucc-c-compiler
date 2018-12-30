// RUN: %ucc -fsyntax-only %s
main()
{
	int x;
	void f(__typeof(x) *);

	f(&x);
}
