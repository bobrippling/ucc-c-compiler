
main()
{
	char *const no_inc = "hi";
	const char *inc = "hi";
	const char *const nochange = "hi";

	++ no_inc;
	++*no_inc;

	++ inc;
	++*inc;

	++ nochange;
	++*nochange;
}
