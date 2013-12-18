// RUN: %ucc -o %t %s
// RUN: %ocheck 0 %t
// RUN: %t | %output_check '{ 0, 0, 0, y }' '{ 0, 0, 0, z }' '{ 0, 0, 0, y }' '{ 0, 0, 0, z }' '{ 0, 0, 1, s }' '{ 0, 1, 0, i }'

struct A
{
	int i, j, k;
	char ch;
};

pred(){ return 1; }

print_A(struct A const *p)
{
	printf("{ %d, %d, %d, %c }\n", p->i, p->j, p->k, p->ch);
}

main()
{
	struct A y = { .ch = 'y' }, z = { .ch = 'z' };
	struct A x = pred() ? y : z;

	/*struct A cst = (struct A)x;*/

	struct A comma = (y, z);

	struct A exp_stmt = ({ struct A sub = { .k = 1, .ch = 's' }; sub; });

	struct A init = (struct A){ .j = 1, .ch = 'i' };

	(void)x;

	print_A(&y);
	print_A(&z);
	print_A(&x);
	print_A(&comma);
	print_A(&exp_stmt);
	print_A(&init);

	return 0;
}
