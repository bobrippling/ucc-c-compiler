// ucc -g tim.c

_Noreturn void abort()
{
	__builtin_unreachable();
}

void realloc()
{
	int local = 5;
	abort();
}
