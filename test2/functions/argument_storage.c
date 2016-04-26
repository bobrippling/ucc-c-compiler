// RUN: %check -e %s

int f(register int i) // CHECK: !/error/
{
	return i;
}

int g(i)
	register int i; // CHECK: !/error/
{
	return i;
}

int h(static int i) // CHECK: error: static storage on "i"
{
	return i;
}

int i(i)
	static int i; // CHECK: error: static storage on "i"
{
	return i;
}

int j(extern int i) // CHECK: error: extern storage on "i"
{
	return i;
}

int k(i)
	extern int i; // CHECK: error: extern storage on "i"
{
	return i;
}
