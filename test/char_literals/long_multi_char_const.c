// RUN: %check %s

main()
{
	int a = 'abcde'; // CHECK: warning: ignoring extraneous characters in literal
	printf("%d\n", a);
}
