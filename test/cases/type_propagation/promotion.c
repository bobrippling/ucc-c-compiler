// RUN: %check --only -e %s

void f(int, ...);

void g(void);

int main()
{
	// shouldn't try to cast `void` to anything:
	f(g()); // CHECK: error: argument 1 to f requires non-void expression
	// CHECK: ^error: mismatching types, argument 1 to f

	f(1, g()); // CHECK: error: argument 2 to f requires non-void expression
}
