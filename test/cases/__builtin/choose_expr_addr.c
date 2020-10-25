// RUN: %ocheck 5 %s

g = 5;
int *p = &__builtin_choose_expr(0, (void)0, g);

main()
{
	int k = 5, *i;

	i = &__builtin_choose_expr(1, k, (void)0);

	*i = 3;
	*p = 2;

	return k + g;
}
