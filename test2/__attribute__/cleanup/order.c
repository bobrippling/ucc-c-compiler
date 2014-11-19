// RUN: %ocheck 0 %s

int *expected[2];
int expected_i;
int dints;

void dint(int *p)
{
	if(p != expected[expected_i++])
		abort();
	dints++;
}

void f()
{
	__attribute((cleanup(dint))) __auto_type x = 3;
	__attribute((cleanup(dint))) __auto_type y = 4;

	expected[0] = &y;
	expected[1] = &x;
	expected_i = 0;
}

main()
{
	// 'y' should be cleaned up, then 'x'
	__attribute((cleanup(dint))) __auto_type x = 3;
	__attribute((cleanup(dint))) __auto_type y = 4;

	f();

	expected[0] = &y;
	expected[1] = &x;
	expected_i = 0;

	return 0;
}
