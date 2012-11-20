// RUN: %ucc %s; [ $? -ne 0 ]
// RUN: %ucc %s 2>&1 | %check %s

struct A
{
	int i;
};

main()
{
	struct A *p = (void *)0;

	p.i; // CHECK: /error: '[^']+' is not a struct or union \(member i\)/
}
