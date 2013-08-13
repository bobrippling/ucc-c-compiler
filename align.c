p(char *p
#ifdef WITH_ALIGN
		, char *align
#endif
		)
{
	printf("hello\n");
}

main()
{
	p(0
#ifdef WITH_ALIGN
			, 0
#endif
			);
}
