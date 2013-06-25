f(struct A);
struct A get();

main()
{
	struct A x = pred() ? y : z;

	struct A cst = (struct A)x;

	struct A comma = cst, x;

	struct A exp_stmt = ({ struct A sub = { 1 }; sub; });

	struct A init = (struct A){ 1 };

	(void)x;
}
