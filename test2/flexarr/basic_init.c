// RUN: %ucc -fsyntax-only %s
struct A
{
	int n;
	int x[];
};

main()
{
	struct A fine = {
		1
	};

	static struct A y = {
		5,
		1, 2, 3, 4, 5
	};

	_Static_assert(
			__builtin_types_compatible_p(typeof(y), struct A),
			"completed array?");
}
