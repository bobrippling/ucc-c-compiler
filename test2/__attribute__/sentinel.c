f(int, ...           ) __attribute__((sentinel));
g(int, ...           ) __attribute__((sentinel(0))); // same
h(int, ... /*, int */) __attribute__((sentinel(1)));
k(int, ...           ) __attribute__((sentinel(3)));

//q(int, ...) __attribute__((sentinel(-3))); - parse error
r(int) __attribute__((sentinel));

main()
{
	f(1, 2, 3);
	f(1, (void *)0);
	f(1, 2, 3, (void *)0);
	f(1, 2, 3, (void *)0, 2);
	f(1, 0); // not null-ptr-const

	f(1, 2, 3, (void *)0, 5, 6, 7, (void *)0); // fine
	f(1, 2, 3, (void *)0, 5, 6, 7, (void *)0, 2); // not fine

	g(1, 0);
	g(0);

	h(5);
	h(5, (void *)0);
	h(5, (void *)0, 2, 3);
	h(5, (void *)0, 2);

	k(1);
	k(1, 2, (void *)0, 3, 3, 3);
	k(1, 2, (void *)0, 3, 3, 3, 4);

	//q(5, (void *)0);
	r(5);
}
