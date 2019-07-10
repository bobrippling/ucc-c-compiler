// RUN: %ocheck 0 %s
int printf(const char *, ...) __attribute__((format(printf, 1, 2)));

void _Noreturn abort(void);

f()
{
	static called;
	if(!called){
		called = 1;
		printf("f()\n");
		return 5;
	}
	abort();
}

g()
{
	long clobber[] = { 1, 2, 3 };
	(void)clobber;
}

main()
{
	typedef unsigned short buf[f()];

	for(int i = 0; i < 10; i++){
		buf b, c;

		g();
	}

	return 0;
}
