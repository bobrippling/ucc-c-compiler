// RUN: %ocheck 0 %s
void abort(void) __attribute__((noreturn));

main()
{
	unsigned a, b;
	short s;

	a = 0xf000;
	b = (short)a;

	if(b != 0xfffff000)
		abort();

	a = 0xf0f0;
	b = (char)a;

	if(b != 0xfffffff0)
		abort();

	return 0;
}
