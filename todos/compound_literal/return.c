main()
{
	return ({
			(union {__typeof(1) i, j;}){.i = 5}.j;
		});
}
