// RUN: %ocheck 0 %s
void abort(void) __attribute__((noreturn));

fs;

f()
{
	fs++;
}

main()
{
	float x = 5;
	if(x)
		f();

	if(2.3)
		f();

	if(fs != 2)
		abort();

	return 0;
}
