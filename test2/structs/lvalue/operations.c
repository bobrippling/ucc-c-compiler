// RUN: %ocheck 0 %s

struct A
{
	int i, j, k;
	char c;
};

pred(){ return 1; }

check_A(
		struct A const *p,
		int i, int j, int k, char c)
{
	if(p->i != i
	|| p->j != j
	|| p->k != k
	|| p->c != c)
	{
		abort();
	}
}

main()
{
	struct A y = { .c = 'y' }, z = { .c = 'z' };
	struct A x = pred() ? y : z;

	/*struct A cst = (struct A)x;*/

	struct A comma = (y, z);

	struct A exp_stmt = ({ struct A sub = { .k = 1, .c = 's' }; sub; });

	struct A init = (struct A){ .j = 1, .c = 'i' };

	(void)x;

	check_A(&y,        0, 0, 0, 'y');
	check_A(&z,        0, 0, 0, 'z');
	check_A(&x,        0, 0, 0, 'y');
	check_A(&comma,    0, 0, 0, 'z');
	check_A(&exp_stmt, 0, 0, 1, 's');
	check_A(&init,     0, 1, 0, 'i');

	return 0;
}
