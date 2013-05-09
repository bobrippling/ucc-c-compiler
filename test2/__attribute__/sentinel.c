// RUN: %ucc -c %s
// RUN: %check %s

f(int, ...           ) __attribute__((sentinel));
g(int, ...           ) __attribute__((sentinel(0))); // same
h(int, ... /*, int */) __attribute__((sentinel(1)));
k(int, ...           ) __attribute__((sentinel(3)));

//q(int, ...) __attribute__((sentinel(-3))); - parse error
r(int) __attribute__((sentinel)); // CHECK: /warning: variadic function required for sentinel check/

main()
{
	f(1, 2, 3); // CHECK: /warning: sentinel argument expected \(got int\)/
	f(1, (void *)0);
	f(1, 2, 3, (void *)0);
	f(1, 2, 3, (void *)0, 2); // CHECK: /warning: sentinel argument expected \(got int\)/
	f(1, 0);  // CHECK: /warning: sentinel argument expected \(got int\)/
	//   ^ not null-ptr-const

	f(1, 2, 3, (void *)0, 5, 6, 7, (void *)0);
	f(1, 2, 3, (void *)0, 5, 6, 7, (void *)0, 2);  // CHECK: /warning: sentinel argument expected \(got int\)/

	g(1, 0); // CHECK: /warning: sentinel argument expected \(got int\)/
	g(0);    // CHECK: /warning: not enough variadic arguments for a sentinel/

	h(5);            // CHECK: /warning: not enough variadic arguments for a sentinel/
	h(5, (void *)0); // CHECK: /warning: sentinel index is not a variadic argument/
	h(5, (void *)0, 2, 3); // CHECK: /warning: sentinel argument expected \(got int\)/
	h(5, (void *)0, 2);

	k(1); // CHECK: /warning: not enough variadic/
	k(1, 2, (void *)0, 3, 3, 3);
	k(1, 2, (void *)0, 3, 3, 3, 4); // CHECK: /warning: sentinel argument expected \(got int\)/

	//q(5, (void *)0);
	r(5);
}
