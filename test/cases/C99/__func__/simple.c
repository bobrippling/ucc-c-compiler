// RUN: %ocheck 0 %s
void abort(void) __attribute__((noreturn));

const char (*f())[]
{
	return &__func__;
}

const char (*g())[]
{
	return &__FUNCTION__;
}

main()
{
#include "../../ocheck-init.c"
	const char *s = f();
	if(s[0] != 'f' || s[1])
		abort();

	s = g();
	if(s[0] != 'g' || s[1])
		abort();

	return 0;
}
