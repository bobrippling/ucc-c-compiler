// RUN: %check %s -Wall
struct A
{
	char c;
	int i; // CHECK: /warning: padding 'A' with 3 bytes to align 'int i'/
};

main()
{
	struct A a = { 1, 2 };

	return a.c + a.i;
}
