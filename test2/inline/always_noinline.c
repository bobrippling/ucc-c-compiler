// RUN: %check -e %s

__attribute((always_inline, noinline))
int f()
{
	return 3;
}

int main()
{
	return f(); // CHECK: error: can't always_inline noinline function
}
