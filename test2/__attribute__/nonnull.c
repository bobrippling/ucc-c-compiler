// RUN: %check %s

f(int *) __attribute__((nonnull()));
g(int *, int *, int *, int *, int *) __attribute__((nonnull(2, 3, 1, 1, 2)));

h(short) __attribute__((nonnull(1))); // CHECK: /warning: nonnull attribute applied to non-pointer/

q() __attribute__((nonnull)); // CHECK: /warning: nonnull attribute on parameterless function/

r(int i, int j) __attribute__((nonnull(3))); // CHECK: /warning: nonnull attributes above argument index 3 ignored/

r(int i, int j) __attribute__((nonnull(1))); // CHECK: /warning: nonnull attribute applied to non-pointer argument 'int'/

main()
{
	f((void *)0); // CHECK: /warning: null passed where non-null required \(arg 1\)/
}
