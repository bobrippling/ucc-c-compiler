// RUN: %check %s -Wall

static char c; // CHECK: warning: unused variable 'c'

static int f() // CHECK: warning: unused function 'f'
{
	return 3;
}

void d(){} // CHECK: !/warn/

main()
{
	return 5;
}
