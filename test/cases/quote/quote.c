// RUN: %ocheck 0 %s
void abort(void) __attribute__((noreturn));

assert(x)
{
	if(!x)
		abort();
}

main()
{
#include "../ocheck-init.c"
	assert('"' + '\"' == 68);

	assert('/*' == 12074);

	return 0;
}
