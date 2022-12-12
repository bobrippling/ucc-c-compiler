// RUN: %ocheck 0 %s
void abort(void) __attribute__((noreturn));

gs;

g()
{
	gs++;
	return 2;
}

a(int i)
{
	return (int (*[])()){
		[5] = g
	}[i]();
}

b(int i)
{
	static int (**fns)() = (int (*[])()){
		[5] = g
	};
	return fns[i]();
}

c(int i)
{
	static __auto_type fns = (int (*[])()){
		[5] = g
	};
	return fns[i]();
}

assert(_Bool b)
{
	if(!b)
		abort();
}

main()
{
#include "../../ocheck-init.c"
	assert(gs == 0);

	assert(a(5) == 2);
	assert(b(5) == 2);
	assert(c(5) == 2);

	assert(gs == 3);

	return 0;
}
