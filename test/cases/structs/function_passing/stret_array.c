// RUN: %ocheck 0 %s
void abort(void) __attribute__((noreturn));

typedef struct
{
	int buf[4];
} A;

A f(void)
{
	return (A){{ 1, 2, 3 }};
}

int main()
{
	A a = f();
	if(a.buf[0] != 1
	|| a.buf[1] != 2
	|| a.buf[2] != 3
	|| a.buf[3] != 0)
	{
		abort();
	}

	return 0;
}
