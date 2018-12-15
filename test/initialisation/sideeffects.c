// RUN: %check %s -Woverride-init-side-effects
// RUN: %ocheck 0 %s

void assert(_Bool b)
{
	if(!b){
		_Noreturn void abort(void);
		abort();
	}
}

int main()
{
	int x = 1, y = 1, z = 1;

	int a[] = {
		[0 ...5] = x++, // CHECK: note: overwritten initialiser here
		[2 ...6] = y++, // CHECK: warning: initialiser with side-effects overwritten
		[0 ...4] = z++, // CHECK: warning: initialiser with side-effects overwritten
	};
	_Static_assert(sizeof(a)/sizeof(*a) == 7, "");

	assert(x == 1);
	assert(y == 2);
	assert(z == 2);

	int n = sizeof(a)/sizeof(*a);
	for(int i = 0; i < n; i++)
		assert(a[i] == 1);
}
