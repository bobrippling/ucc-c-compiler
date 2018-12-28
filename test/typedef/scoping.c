// RUN: %ucc %s -o %t
// RUN: %t

typedef int td_val;

main()
{
	typedef short td_val;

	td_val i;

	if(sizeof(i) != 2)
		abort();
	return 0;
}
