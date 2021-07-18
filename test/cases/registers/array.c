// RUN: %check --only -e %s

void f(int (*)[]);

int main()
{
	register int a[] = { 1, 2, 3 };

	f(&a); // CHECK: error: can't take the address of register

	return a[1];
}
