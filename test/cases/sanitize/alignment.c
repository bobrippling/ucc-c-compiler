// RUN: %ocheck 5 %s -fsanitize=alignment -fsanitize-error=call=san_err

int ec = 0;

void san_err(void)
{
	__attribute((noreturn))
	void exit(int);
	exit(ec);
}

int f(int *p)
{
	return *p;
}

int main()
{
	int a = 3;

	ec = 0;
	f(&a); // should be able to load normally

	ec = 5;
	f((char *)&a + 2); // shouldn't be able to load from unaligned pointers
};
