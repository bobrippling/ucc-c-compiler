// RUN: %check -e %s
struct A; // CHECK: /note: forward declared here/

main()
{
	struct A a; // CHECK: error: "a" has incomplete type 'struct A'
}
