// RUN: %check %s -fshow-inlined -finline-functions
// RUN: %ocheck 3 %s -finline-functions

static void f(int const *p)
{
	*(int *)p = 3;
}

int main()
{
	int i;
	f(&i); // CHECK: note: function inlined
	return i;
}
