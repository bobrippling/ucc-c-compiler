typedef struct B
{
	int k;
} B;

struct A
{
	int i;
	B;
};

main()
{
	struct A p;
	f(&p);
	/* extension #2 - typedef name used for member
	 * only if not ambiguous */
	return p.B.k;
}
