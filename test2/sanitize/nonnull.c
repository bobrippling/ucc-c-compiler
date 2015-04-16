// RUN: %ocheck 5 %s -fsanitize=undefined -fsanitize-error=call=san_fail

int ec = 7;

void san_fail(void)
{
	extern _Noreturn void exit(int);
	exit(ec);
}

f(int *p)
	__attribute((nonnull))
{
	return *p;
}

main()
{
	ec = 0;

	f(&(int){ 2 });

	ec = 5;

	f((void *)0);

	return 3;
}
