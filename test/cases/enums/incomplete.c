// RUN: %check -e %s

enum A; // CHECK: /warning: forward-declaration of enum A/

main()
{
	enum A a; // CHECK: error: "a" has incomplete type 'enum A'
}
