struct A
{
	int i, j, k;
};

f(struct A);
struct A get();

pred(){ return 1; }

print_A(struct A const *p)
{
	printf("{ %d, %d, %d }\n", p->i, p->j, p->k);
}

main()
{
	struct A y = {}, z = {};
	struct A x = pred() ? y : z;

	/*struct A cst = (struct A)x;*/

	struct A comma = (y, z);

	struct A exp_stmt = ({ struct A sub = { .k = 1 }; sub; });

	struct A init = (struct A){ .j = 1 };

	(void)x;

	print_A(&y);
	print_A(&z);
	print_A(&x);
	print_A(&comma);
	print_A(&exp_stmt);
	print_A(&init);
}
