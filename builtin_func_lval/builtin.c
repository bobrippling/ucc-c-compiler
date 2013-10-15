main()
{
	int k, *i;

	i = &__builtin_choose_expr(1, k, (void)0);

	*i = 3;

	return k;
}
