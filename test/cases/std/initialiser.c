// RUN: %check %s -std=c89

main()
{
	/* C89 3.5.7 */
	int x[] = { f(), f(), f() }; // CHECK: /warning: aggregate initialiser is not a constant expression/
	struct { int i, j, k; } y = { f(), f(), f() }; // CHECK: /warning: aggregate initialiser is not a constant expression/
}
