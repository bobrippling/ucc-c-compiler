// RUN: %check -e %s

f(int n)
{
	char a[n], b[n * 2];

	_Static_assert(sizeof(a) != sizeof(b), "hi"); // CHECK: error: static assert: not an integer constant expression (operator)
}
