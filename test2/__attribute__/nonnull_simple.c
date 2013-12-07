// RUN: %ucc -c %s
// RUN: %check %s

f(int *) __attribute__((nonnull));

main()
{
	int i;

	f((void *)0); // CHECK: /warning: null passed where non-null required/
	f(&i);
	f(0); // CHECK: /warning: null passed where/
}
