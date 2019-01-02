// RUN: %check -e %s

__attribute((always_inline, weak))
int f()
{
	return 3;
}

int main()
{
	return f(); // CHECK: error: couldn't always_inline call: weak-function overridable at link time
}
