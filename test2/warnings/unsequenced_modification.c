// RUN: %check %s

void f(int, int);
void g(int, int, int, int);

int main()
{
	int i = 0; // CHECK: !/warn/

	// ensure init-then-assign doesn't warn
	i = 1; // CHECK: !/warn/

	f(i, i = 2); // CHECK: warning: unsequenced modification of "i"

	g(i, i, i, i); // CHECK: !/warn/

	i = 1, i = 2; // CHECK: !/warn/

	i = i++; // CHECK: warning: unsequenced modification of "i"

	i = i + 1; // CHECK: !/warn/
}
