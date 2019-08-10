// RUN: %ocheck 7 %s -fsanitize=returns-nonnull-attribute -fsanitize-error=call=san_err

void san_err(void)
{
	__attribute((noreturn))
	void exit(int);

	exit(7);
}

__attribute__((returns_nonnull))
int *f()
{
	return 0;
}

int main()
{
	f();
}
