// RUN: %check %s

printf1(char *, ...) __attribute((format(printf, 2, 1))); // CHECK: warning: format argument out of bounds (1 >= 1)
printf2(char *, ...) __attribute((format(printf, 1, 2)));

main()
{
	// checks disabled for printf
	printf1("hi %d\n", "yo"); // CHECK: !/warn/
	printf2("hi %d\n", "yo"); // CHECK: warning: format %d expects integral argument (got char *)
}
