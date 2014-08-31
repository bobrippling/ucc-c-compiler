main()
{
	char buf[16];

	__asm(
			"cld\n"
			"rep\n"
			"movsb"
			:
			: "S"("hello"), "D"(buf), "c"(6));

	printf("%s\n", buf);
}
