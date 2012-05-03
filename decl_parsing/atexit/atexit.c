p_hi()
{
	printf("beet\n");
}

p_bye()
{
	printf("bye\n");
}

main()
{
	atexit(p_hi);
	atexit(p_bye);
}
