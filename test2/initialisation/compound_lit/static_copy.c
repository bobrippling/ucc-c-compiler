// RUN: %check %s
// RUN: %ocheck 0 %s

struct A
{
	int i, j;
};

struct A a = (struct A){ 1, 2 }; // CHECK: warning: statically initialising with compound literal (not a constant expression)

_Noreturn void abort(void);

int main()
{
	if(a.i != 1)
		abort();
	if(a.j != 2)
		abort();

	return 0;
}
