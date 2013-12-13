// RUN: %check -e %s

struct A
{
	int i;
};

struct B;

main()
{
	__builtin_is_signed(struct A); // CHECK: /error: __builtin_is_signed on non-scalar type 'struct A'/
	__builtin_is_signed(struct B); // CHECK: /error: __builtin_is_signed on incomplete type 'struct B'/
}
