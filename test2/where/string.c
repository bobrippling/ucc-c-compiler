// RUN: %check %s
typedef long intptr_t;

static struct
{
	intptr_t p;
} abc = {
	.p = "hi" "there" // CHECK: /warning: mismatching types/
}; // CHECK: !/warn/

main()
{
	int f(int);
	f("hello" "there" // CHECK: /warning: mismatching types/
	 ); // CHECK: !/warn/
}
