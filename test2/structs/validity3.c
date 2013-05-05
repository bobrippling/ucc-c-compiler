// RUN: %ucc %s; [ $? -ne 0 ]
// RUN: %check %s

struct A
{
	int i;
};

main()
{
	struct A a;

	a->i; // CHECK: /error: '[^']+' is not a pointer to struct or union \(member i\)/
}
