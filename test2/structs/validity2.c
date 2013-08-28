// RUN: %check -e %s

struct A
{
	int i;
};

main()
{
	struct A *p = (void *)0;

	p.i; // CHECK: /error: .* is not a struct or union/
}
