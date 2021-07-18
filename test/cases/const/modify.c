// RUN: %check -e %s

main()
{
	char *const no_inc = "hi";
	const char *inc = "hi";
	const char *const nochange = "hi";

	++ no_inc; // CHECK: /error: can't modify/
	++*no_inc;

	++ inc;
	++*inc; // CHECK: /error: can't modify/

	++ nochange; // CHECK: /error: can't modify/
	++*nochange; // CHECK: /error: can't modify/
}
