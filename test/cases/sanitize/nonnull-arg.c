// RUN: %ocheck 3 %s -fsanitize=nonnull-attribute -fsanitize-error=call=san_err

__attribute((nonnull(2)))
void f(int *a, int *b, int *c)
{
}

static int ec;

void san_err(void)
{
	__attribute((noreturn))
	void exit(int);

	exit(ec);
}

int main()
{
#include <ocheck-init.c>
	ec = 1;

	f(0, 1, 0);
	f(0, &(int){}, 0);

	ec = 3;
	f(0, 0, 0);
}
