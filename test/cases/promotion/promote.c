// RUN: %ocheck 0 %s
void abort(void) __attribute__((noreturn));

main()
{
	unsigned char a = 0xff;
	char b = 0xff;
	int c = a == b;

	if(c)
		abort();

	return 0;
}
