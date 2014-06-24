// RUN: %check -e %s

struct A;

struct A
{
	struct A x; // CHECK: error: incomplete field 'struct A x'
};

main()
{
}
