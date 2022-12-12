// RUN: %check %s
// RUN: %ocheck 0 %s
void abort(void) __attribute__((noreturn));

enum {
	A = 'abc ', // CHECK: /warning: multi-char constant/
	B = 'xyz?', // CHECK: /warning: multi-char constant/
	C = '3251', // CHECK: /warning: multi-char constant/
	D = '500B', // CHECK: /warning: multi-char constant/
};

main()
{
#include "../ocheck-init.c"
	// check escapes inside m-char constants
	if('a\'' != 24871) // CHECK: /warning: multi-char constant/
		abort();

	return 0;
}
