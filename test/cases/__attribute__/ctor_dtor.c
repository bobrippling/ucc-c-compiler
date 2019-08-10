// RUN: %ocheck 2 %s

int inited;

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

_Noreturn void abort();

main()
{
	if(!inited)
		abort();

	return 1;
}
