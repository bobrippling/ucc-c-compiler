// RUN: %check %s

typedef _Bool bool;

int f(bool b)
{
	return b;
}

g(int *p)
{
	// bool implicitly convertible from pointer
	return f(p); // CHECK: !/warn/
}
