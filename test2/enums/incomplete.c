// RUN: %check -e %s

enum A; // CHECK: /warning: predeclaration of enums is not C99/

main()
{
	enum A a; // CHECK: /error: use of incomplete enum A/
}
