// RUN: %check %s
typedef long intptr_t;

static struct
{
	intptr_t p;
} abc = {
	.p = "hi" // CHECK: /warning: mismatching types/
}; // CHECK: !/warn/

main()
{
	int f(int);
	f("hello" // CHECK: /warning: mismatching argument/
	 ); // CHECK: !/warn/
}
