// RUN: %check %s

void f(int, int);
void g(int, int, int, int);
int h(int);

int main()
{
	int i = 0; // CHECK: !/warn/
	int a = 3; // CHECK: !/warn/
	int b = h(a); /* used to get a unseq for 'a' here */ // CHECK: !/warn/
	int c = (b = b++); // CHECK: unsequenced modification of "b"

	// ensure init-then-assign doesn't warn
	i = 1; // CHECK: !/warn/
	i += 1; // CHECK: !/warn/

	f(i, i = 2); // CHECK: warning: unsequenced modification of "i"
	f(i, i += 2); // CHECK: warning: unsequenced modification of "i"

	g(i, i, i, i); // CHECK: !/warn/

	i = 1, i = 2; // CHECK: !/warn/
	i += 1, i += 2; // CHECK: !/warn/

	i = i++; // CHECK: warning: unsequenced modification of "i"
	i += i++; // CHECK: warning: unsequenced modification of "i"

	i = i + 1; // CHECK: !/warn/
	i += i + 1; // CHECK: !/warn/

	for(i = 0; i < 10; i++); // CHECK: !/warn/
}
