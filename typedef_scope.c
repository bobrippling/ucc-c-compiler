typedef int td_val;

main()
{
	typedef short td_val;

	td_val i;

	(void)_Generic(i, td_val: 0);
}
