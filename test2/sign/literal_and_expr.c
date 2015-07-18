// RUN: %check %s -Wsign-compare

int f(int i)
{
	if(g())
		return i < sizeof(int); // CHECK: warning: signed and unsigned types in '<'

	return 3 < 5u-g(); // CHECK: warning: signed and unsigned types in '<'
}
