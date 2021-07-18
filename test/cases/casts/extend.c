// RUN: %check %s

printf();

main()
{
	signed char c = -1; // CHECK: !/warn|err/
	signed long i = -1; // CHECK: !/warn|err/

	(void)c;
	(void)i;

	return 0;
}
