f(int *) __attribute__((nonnull()));
g(int *, int *, int *, int *, int *) __attribute__((nonnull(2, 3, 1, 1, 2)));

h(short) __attribute__((nonnull(1))); // should warn

q() __attribute__((nonnull)); // should warn

main()
{
	f((void *)0); // should warn
}
