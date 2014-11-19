// RUN: %ucc -fsyntax-only -std=c99 %s
// RUN: %check -e %s -std=c89

struct A
{
	int ar[2];
};

struct A a(void);
f(int *p);

main()
{
	f(a().ar); // CHECK: error: mismatching types, argument 1 to f:
}
