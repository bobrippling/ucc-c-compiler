struct A
{
	int i, j, k;
};

f(struct A);
struct A get();

main()
{
	struct A y = {}, z = {};
	struct A x = pred() ? y : z;

	//struct A cst = (struct A)x;

	struct A comma = (y, x);

	struct A exp_stmt = ({ struct A sub = { 1 }; sub; });

	struct A init = (struct A){ 1 };

	(void)x;
}
