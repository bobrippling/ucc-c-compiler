// RUN: %ucc %s 2>&1 | grep -E 'warn|error'; [ $? -eq 1 ]
printf();

main()
{
	signed char c = -1;
	signed long i = -1;

	(void)c;
	(void)i;

	return 0;
}
