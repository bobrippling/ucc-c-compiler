// RUN: %check %s -Wgnu
// RUN: %ocheck 0 %s

void abort(void);

main()
{
	int i = {}; // CHECK: warning: use of GNU empty initialiser
	struct A { int i; } a = {}; // CHECK: warning: use of GNU empty initialiser
	struct B { int i, j; } b = {}; // CHECK: warning: use of GNU empty initialiser

	if(i)
		abort();

	if(a.i)
		abort();

	if(b.i || b.j)
		abort();

	return 0;
}
