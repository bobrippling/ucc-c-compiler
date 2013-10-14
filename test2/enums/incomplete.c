// RUN: %check -e %s

enum A; // CHECK: /warning: forward-declaration of enum A/

main()
{
	enum A a; // CHECK: /error: enum A is incomplete/
}
