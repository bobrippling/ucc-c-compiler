// RUN: %ocheck 0 %s
void abort(void) __attribute__((noreturn));

typedef unsigned long size_t;

void f(size_t n)
{
  /* n must be incremented */
  size_t a = sizeof(int[++n]);

  /* n need not be incremented - no change on size */
  size_t b = sizeof(int[++n % 1 + 1]);

	if(a != sizeof(int) * 3)
		abort();
	if(b != sizeof(int))
		abort();
}

main()
{
	f(2);

	return 0;
}
