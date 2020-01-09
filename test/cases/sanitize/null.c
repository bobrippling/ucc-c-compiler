// RUN: %ocheck 5 %s -fsanitize=null -fsanitize-error=call=san_err

int ec;

void san_err(void)
{
	void exit(int) __attribute((noreturn));
	exit(ec);
}

void use(int *p)
{
	int x = *p;
	(void)x;
}

main()
{
	int i = 3;
	int *p = &i;

	ec = 0;
	use(p);

	ec = 5;
	p = 0;
	use(p);
}
