typedef signed t;

struct A
{
	unsigned t; // "t" of type unsigned int
	const t; // useless decl
	t i; // hi
};

main()
{
	struct A x;
	x.i = 5;
}
