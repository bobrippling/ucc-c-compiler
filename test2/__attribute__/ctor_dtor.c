// RUN: %ocheck 2 %s

int inited;
int f_called;

__attribute((constructor))
static void ct()
{
	inited = 1;
}

__attribute((destructor))
static void dt()
{
	void _exit(int);

	_exit(2);
}

main()
{
	if(!inited)
		abort();

	if(!f_called)
		abort();

	void f() __attribute((constructor));

	return 1;
}

void f()
{
	f_called = 1;
}
