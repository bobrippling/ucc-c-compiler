// RUN: %check -e %s -DERROR
// RUN: %ocheck 0 %s

void abort(void);

void assert(int x)
{
	if(!x)
		abort();
}

char good;
char bad;

void f()
{
	extern char good; // CHECK: !/error/
	good = 1;
}

void g()
{
	int good = 5; // CHECK: !/error/
	{
		extern char good; // CHECK: !/error/
		good = 2;
	}
	if(good != 5)
		abort();
}

void h()
{
	static int good; // CHECK: !/error/
	good = 2;
}

main()
{
#ifdef ERROR
	/* section 6.1.2.2 */
	extern int bad; // CHECK: error: incompatible redefinition of "bad"

	return bad;
#endif

	assert(good == 0);
	assert(bad == 0);

	f();
	assert(good == 1);
	assert(bad == 0);

	g();
	assert(good == 2);
	assert(bad == 0);

	good = 10;
	h();
	assert(good == 10);
	assert(bad == 0);
}
