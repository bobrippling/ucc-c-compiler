// RUN: %jmpcheck %s

void t(), f();
int test();

likely()
{
	if(__builtin_expect(test(), 1))
		t();
	else
		f();
}

unlikely()
{
	if(__builtin_expect(test(), 0))
		t();
	else
		f();
}
