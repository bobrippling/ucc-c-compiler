// RUN: %ocheck 0 %s
void abort(void) __attribute__((noreturn));

struct A
{
	int *p;
	int how;
};

main()
{
#include "../ocheck-init.c"
	static struct A a = {
		// ensure &a.how uses the correctly mangled name of 'a' (static)
		.p = &a.how
	};

	if(a.p != &a.how)
		abort();
	if(a.how)
		abort();
}
