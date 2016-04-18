// RUN: %check %s
int printf(const char *, ...) __attribute__((format(printf, 1, 2)));

main()
{
	int a = 'abcde'; // CHECK: warning: ignoring extraneous characters in literal
	printf("%d\n", a);
}
