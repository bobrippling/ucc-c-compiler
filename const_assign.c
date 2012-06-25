f(int i)
{
	(void)i;
}

main()
{
	const char c = 2;
	const char *s = (char *)0;
	char *s2 = "hi";
	char *s3 = (const char *)0;
	f(c);

	(void)s;
	(void)s2;
	(void)s3;
}
