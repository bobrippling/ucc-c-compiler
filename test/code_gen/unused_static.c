// RUN: %check %s

static char c; // CHECK: warning: unused global variable 'c'

static int f() // CHECK: warning: unused global function 'f'
{
	return 3;
}

void d(){} // CHECK: !/warn/

main()
{
	return 5;
}
