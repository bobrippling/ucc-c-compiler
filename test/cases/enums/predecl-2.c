// RUN: %check -e %s

enum A;

f(enum A a); // CHECK: error: incomplete parameter type 'enum A'

main()
{
	f(2); // CHECK: note: in call here
}
