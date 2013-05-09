// RUN: %ucc %s; [ $? -ne 0 ]
// RUN: %check %s

struct A
{
	int i;
};

main()
{
	struct A *p = (void *)0;

	p.i; // CHECK: /error: '[^']+' is not a struct or union \(member i\)/
}
