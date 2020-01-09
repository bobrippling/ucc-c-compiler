// RUN: %ocheck 7 %s -fsanitize=returns-nonnull-attribute -fsanitize-error=call=san_err

int ec;

void san_err(void)
{
	__attribute((noreturn))
	void exit(int);

	exit(ec);
}

int *p;

__attribute__((returns_nonnull))
int *f()
{
	return p;
}

int main()
{
	ec = 0;
	p = &(int){ 0 };
	f();

	ec = 7;
	p = 0;
	f();
}
