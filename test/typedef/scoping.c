// RUN: %ucc %s -o %t
// RUN: %t
void abort(void) __attribute__((noreturn));

typedef int td_val;

main()
{
	typedef short td_val;

	td_val i;

	if(sizeof(i) != 2)
		abort();
	return 0;
}
